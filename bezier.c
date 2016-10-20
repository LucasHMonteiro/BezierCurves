#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"

#define ID_FILE_OPEN 9001
#define ID_FILE_SAVE 9002
#define ID_FILE_EXIT 9003
#define ID_FILE_RESIZE 9004

#define PI 3.14159265
#define IDC_MAIN_EDIT 101

double window_sizes[2];
int draw_convex_hull = 0;
int draw_points = 0;
int control_points_size = 4;
POINT* control_points;
int curr_point = 0;
RECT viewport_rect;
RECT window_rect;

void print_points(int** points, int n){
	int i;
	printf("\n\n\n");
	printf("Max X Viewport: %d\n", GetSystemMetrics(SM_CXSCREEN));
	printf("Max Y Viewport: %d\n\n", GetSystemMetrics(SM_CYSCREEN));

	for (i = 0; i < n; i++){
		printf("x: %d y: %d\n", points[i][0], points[i][1]);
	}
	printf("\n\n\n");
}

void init_points(){
	int i;
	for (i = 0; i < control_points_size; i++){
		control_points[i].x = -1;
		control_points[i].y = -1;
	}
}

double* getPoint(){
	double* point = (double*)malloc(sizeof(double)*2);
	printf("The x coordinate: ");
	scanf("%lf", &point[0]);
	printf("    The y coordinate: ");
	scanf("%lf", &point[1]);
	return point;
}

double** getPoints(int n){
	double** points = (double**)malloc(sizeof(double*)*n);
	int i;
	printf("\n Now specify the %d points\n\n", n);
	for (i = 0; i < n; i++){
		printf("Point %d:\n    ", i+1);
		points[i] = getPoint();
	}
	return points;
}

int is_inside_window(int x, int y){
	if(x > window_rect.right || x < window_rect.left || y > window_rect.bottom + 45 || y < window_rect.top + 45){
		return 0;
	}
	return 1;
}

void draw_pixel(int x, int y, HDC hdc){
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, x+1, y);
	MoveToEx(hdc, x, y, NULL);
}

void draw_line(int x1, int y1, int x2, int y2, HDC hdc){
	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);
}

void draw_circle_crazy(int x, int y, double radius, HDC hdc){
	int t;
	for (t = 0; t < 360; t += 5){
		if(!t){
			draw_pixel(x+radius*cos(t), y+radius*sin(t), hdc);
		}else{
			LineTo(hdc, x+radius*cos(t), y+radius*sin(t));
		}
	}
}

void draw_circle(int x, int y, double radius, HDC hdc){
	double t;
	for (t = 0; t < 2*PI; t += 0.05){
		if(!t){
			draw_pixel(x+radius*cos(t), y+radius*sin(t), hdc);
		}else{
			LineTo(hdc, x+radius*cos(t), y+radius*sin(t));
		}
	}
	LineTo(hdc, x+radius*cos(t), y+radius*sin(t));
}

void draw_filled_circle(int x, int y, double radius, HDC hdc){
	double t;
	double r;
	for(r = 0.01; r <= radius; r+=0.01){
		for (t = 0; t < 2*PI; t += 0.05){
			if(!t){
				draw_pixel(x+r*cos(t), y+r*sin(t), hdc);
			}else{
				LineTo(hdc, x+r*cos(t), y+r*sin(t));
			}
		}
		LineTo(hdc, x+r*cos(t), y+r*sin(t));
	}
}

void draw_circle_fast(int x0, int y0, double radius, HDC hdc){
	int x = (int)radius;
    int y = 0;
    int err = 0;

    while (x >= y){
        draw_pixel(x0 + x, y0 + y, hdc);
        draw_pixel(x0 + y, y0 + x, hdc);
        draw_pixel(x0 - y, y0 + x, hdc);
        draw_pixel(x0 - x, y0 + y, hdc);
        draw_pixel(x0 - x, y0 - y, hdc);
        draw_pixel(x0 - y, y0 - x, hdc);
        draw_pixel(x0 + y, y0 - x, hdc);
        draw_pixel(x0 + x, y0 - y, hdc);

        y += 1;
        err += 1 + 2*y;
        if (2*(err-x) + 1 > 0)
        {
            x -= 1;
            err += 1 - 2*x;
        }
    }
}

void draw_filled_circle_fast(int x0, int y0, double radius, HDC hdc){
	int x = (int)radius;
    int y = 0;
    int err = 0;

    while (x >= y){
        draw_line(x0 + x, y0 + y, x0 + x, (y0 - y)+1, hdc);
        draw_line(x0 + y, y0 + x, x0 + y, (y0 - x)+1, hdc);
        draw_line(x0 - y, y0 + x, x0 - y, (y0 - x)+1, hdc);
        draw_line(x0 - x, y0 + y, x0 - x, (y0 - y)+1, hdc);
 
        y += 1;
        err += 1 + 2*y;
        if (2*(err-x) + 1 > 0){
            x -= 1;
            err += 1 - 2*x;
        }
    }
}

void draw_rect(int x, int y, int width, int height, HDC hdc){
	draw_line(x, y, x + width + 1, y, hdc);
	draw_line(x, y, x, y + height + 1, hdc);
	draw_line(x + width, y, x + width, y + height + 1, hdc);
	draw_line(x, y + height, x + width + 1, y + height, hdc);
}

void draw_rect_2(RECT* rect, HDC hdc){
	draw_line(rect->left, rect->top, rect->right+1, rect->top, hdc);
	draw_line(rect->left, rect->top, rect->left, rect->bottom+1, hdc);
	draw_line(rect->left, rect->bottom, rect->right+1, rect->bottom, hdc);
	draw_line(rect->right, rect->top, rect->right, rect->bottom+1, hdc);
}

void draw_filled_rect(int x, int y, int width, int height, HDC hdc){
	if(!width || !height) return;
	draw_rect(x, y, width, height, hdc);
	draw_filled_rect(x+1, y+1, width-1, height-1, hdc);
}

void draw_square(int x, int y, int side, HDC hdc){
	draw_rect(x, y, side, side, hdc);
}

void draw_filled_square(int x, int y, int side, HDC hdc){
	draw_filled_rect(x, y, side, side, hdc);
}

void draw_bezier_curve(HDC hdc, int s){
	int x, y;
	double t;
	for (t = 0; t < 1; t += 0.01){
		x = round(pow((1 - t), 3)*control_points[s+0].x + 3*t*pow((1 - t), 2)*control_points[s+1].x + 3*t*t*(1 - t)*control_points[s+2].x + pow(t, 3)*control_points[s+3].x);
		y = round(pow((1 - t), 3)*control_points[s+0].y + 3*t*pow((1 - t), 2)*control_points[s+1].y + 3*t*t*(1 - t)*control_points[s+2].y + pow(t, 3)*control_points[s+3].y);
		if(!t){
			MoveToEx(hdc, x - 2, y - 45, NULL);
		}else{
			LineTo(hdc, x - 2, y - 45);
		}
	}
	LineTo(hdc, control_points[s+3].x-2, control_points[s+3].y-45);
}

void calculate_window(double width, double height){
	int window_width = viewport_rect.right - viewport_rect.left - 40;
	int window_height = viewport_rect.bottom - viewport_rect.top - 100;
	int rect_width;
	int rect_height;

	if(width/window_width > height/window_height){
		rect_width = window_width;
		rect_height = round(height*rect_width/width);
	}else{
		rect_height = window_height;
		rect_width = round(width*rect_height/height);
	}

	int rect_upper_left_x = window_width/2 - rect_width/2 + 10;
	int rect_upper_left_y = window_height/2 - rect_height/2 + 20;

	window_rect.left = rect_upper_left_x;
	window_rect.right = rect_upper_left_x + rect_width;
	window_rect.top = rect_upper_left_y;
	window_rect.bottom = rect_upper_left_y + rect_height;
}

void draw_point_coordinates(HDC hdc, int x, int y){
	double x_w, y_w;
	x_w = (x - window_rect.left)*window_sizes[0]/(window_rect.right - window_rect.left);
	y_w = (((y - window_rect.bottom) - (window_rect.bottom - window_rect.top))*window_sizes[1]/(-(window_rect.bottom - window_rect.top))) - window_sizes[1];

	int len = 10;
	int decimals = x_w<1?1:0;
	int aux = x_w;
	while(aux > 1){
		decimals++;
		aux /= 10;
	}
	len += decimals;
	decimals = y_w<1?1:0;
	aux = y_w;
	while(aux > 1){
		decimals++;
		aux /= 10;
	}
	len += decimals;
	char* str = (char*)malloc(sizeof(char)*len+1);
	sprintf(str, "(%.2f, %.2f)", x_w, y_w);

	RECT* rect = (RECT*)malloc(sizeof(RECT));
	if(y < 50){
		rect->top = y + 10;
		rect->bottom = y + 30;	
	}else{
		rect->top = y - 30;
		rect->bottom = y - 10;
	}

	if(x < 50){
		rect->right = x + 100;
		rect->left = x;
	}else if(x > viewport_rect.right - 50){
		rect->right = x;
		rect->left = x - 100;
	}else{
		rect->right = x + 50;
		rect->left = x - 50;
	}
	DrawText(hdc, str, len+1, rect, DT_CENTER);

	free(str);
	free(rect);
}

void draw_window(HDC hdc){
	draw_rect(window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, hdc);
}

void freePoints(int** points, int n){
	int i;
	for (i = 0; i < n; i++){
		free(points[i]);
	}
}

const char g_szClassName[] = "myWindowClass";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){

	switch(msg){
		case WM_CREATE:{
			control_points = (POINT*)malloc(sizeof(POINT)*control_points_size);
			init_points();

			HMENU hMenu, hSubMenu;
	        HICON hIcon, hIconSm;

	        hMenu = CreateMenu();

	        hSubMenu = CreatePopupMenu();
	        AppendMenu(hSubMenu, MF_STRING, ID_FILE_NEW, "&New");
	        AppendMenu(hSubMenu, MF_STRING, ID_FILE_OPEN, "&Open");
	        AppendMenu(hSubMenu, MF_STRING, ID_FILE_SAVE, "&Save");
	        AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
	        AppendMenu(hSubMenu, MF_STRING, ID_FILE_RESIZE, "&Resize");
	        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&File");

	        SetMenu(hwnd, hMenu);


	        hIcon = LoadImage(NULL, "bezier_large.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
	        if(hIcon)
	            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	        hIconSm = LoadImage(NULL, "bezier.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
	        if(hIconSm)
	            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
	    }
		break;

		case WM_COMMAND:
			switch(LOWORD(wParam)){
				case ID_FILE_NEW:
					if(control_points[0].x >= 0){
						int id = MessageBox(hwnd, "Deseja Salvar?", "Salvar", MB_YESNOCANCEL);
						if(id == IDYES){
							SendMessage(hwnd, WM_COMMAND, ID_FILE_SAVE, 0);
						}else if(id == IDNO){
							printf("Especifique a nova area de desenho (i.e: 800x600): ");
							scanf("%lfx%lf", &window_sizes[0], &window_sizes[1]);
							calculate_window(window_sizes[0], window_sizes[1]);
							control_points = (POINT*)realloc(control_points, sizeof(POINT)*4);
							control_points_size = 4;
							curr_point = 0;
							init_points();
							InvalidateRect(hwnd, 0, TRUE);
						}
					}else{
						printf("Especifique a nova area de desenho (i.e: 800x600): ");
						scanf("%lfx%lf", &window_sizes[0], &window_sizes[1]);
						calculate_window(window_sizes[0], window_sizes[1]);
						control_points = (POINT*)realloc(control_points, sizeof(POINT)*4);
						control_points_size = 4;
						curr_point = 0;
						init_points();
						InvalidateRect(hwnd, 0, TRUE);
					}

				break;
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_FILE_SAVE:{
					OPENFILENAME ofn;
					char file_name[MAX_PATH] = "";
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwnd;
				    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
				    ofn.lpstrFile = file_name;
				    ofn.nMaxFile = MAX_PATH;
				    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				    ofn.lpstrDefExt = "txt";

				    if(GetSaveFileName(&ofn)){
				        FILE* f = fopen(file_name, "w");
				        fprintf(f, "%lf %lf\n", window_sizes[0], window_sizes[1]);
				        fprintf(f, "%d\n", control_points_size);
				        int i;
						for (i = 0; i < control_points_size; i++){
							fprintf(f, "%d %d ", control_points[i].x, control_points[i].y);
						}
				        fclose(f); 
				    }
				}
				break;
				case ID_FILE_OPEN:{
					OPENFILENAME ofn;
					char file_name[MAX_PATH] = "";
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwnd;
				    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
				    ofn.lpstrFile = file_name;
				    ofn.nMaxFile = MAX_PATH;
				    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				    ofn.lpstrDefExt = "txt";

				    if(GetOpenFileName(&ofn)){
				        FILE* f = fopen(file_name, "r");
				        int n = 0;
				        fscanf(f, "%lf %lf", &window_sizes[0], &window_sizes[1]);
				        calculate_window(window_sizes[0], window_sizes[1]);
				        fscanf(f, "%d", &n);
						control_points = (POINT*)realloc(control_points, n*sizeof(POINT));
						control_points_size = n;
						curr_point = control_points_size - 4;
						int i;
						for (i = 0; i < n; i++){
							fscanf(f, "%d %d", &control_points[i].x, &control_points[i].y);
						}
						int j;
						for (j = curr_point; j < control_points_size; j++){
							control_points[j].x = -1;
							control_points[j].y = -1;
						}
				        fclose(f); 
						InvalidateRect(hwnd, 0, TRUE);
				    }
				}
				break;
				case ID_FILE_RESIZE:
					if(control_points[0].x < 0){
						printf("Especifique a nova area de desenho (i.e: 800x600): ");
						scanf("%lfx%lf", &window_sizes[0], &window_sizes[1]);
						calculate_window(window_sizes[0], window_sizes[1]);
						InvalidateRect(hwnd, 0, TRUE);
					}else{
						MessageBox(hwnd, "Voce nao pode redimensionar o canvas se existirem pontos nele.", "Error", MB_OK | MB_ICONERROR);
					}
				break;
			}
		break;

		case WM_PAINT:{
			HPEN green_pen = CreatePen(PS_SOLID, 1, 0x00ff00);
			HPEN white_dashed_pen = CreatePen(PS_DASH, 1, 0x000000);
			HPEN white_solid_pen = CreatePen(PS_SOLID, 1, 0xffffff);
			HPEN blue_solid_pen = CreatePen(PS_SOLID, 1, 0xff0000);
 			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			SelectObject(hdc, green_pen);
			int i;
			int j;
			draw_window(hdc);

			for (i = 0; i < control_points_size; i++){
				SelectObject(hdc, white_dashed_pen);
				if(control_points[i].x > 0 && control_points[i].y > 0){

					if(i%4 == 0){
						MoveToEx(hdc, control_points[i].x - 2, control_points[i].y - 45, NULL);

					}else if(i%4 == 3){
						if(draw_convex_hull){
							LineTo(hdc, control_points[i].x - 2, control_points[i].y - 45);
							LineTo(hdc, control_points[i - 3].x - 2, control_points[i - 3].y - 45);
						}else{
							MoveToEx(hdc, control_points[i].x - 2, control_points[i].y - 45, NULL);
						}
						SelectObject(hdc, blue_solid_pen);
						draw_bezier_curve(hdc, i - 3);

						if(control_points_size == i+1){
							control_points_size += 4;
							control_points = (POINT*)realloc(control_points, (control_points_size)*sizeof(POINT));
							int k;
							for (k = i+1; k < control_points_size; k++){
								control_points[k].x = -1;
								control_points[k].y = -1;
							}
						}

					}else{
						if(draw_convex_hull){
							LineTo(hdc, control_points[i].x - 2, control_points[i].y - 45);
						}else{
							MoveToEx(hdc, control_points[i].x - 2, control_points[i].y - 45, NULL);
						}
					}
					SelectObject(hdc, white_solid_pen);
					draw_filled_square(control_points[i].x - 4, control_points[i].y - 47, 3, hdc);
					if(draw_points) draw_point_coordinates(hdc, control_points[i].x - 2, control_points[i].y - 45);
				}else{
					break;
				}
			}
			SelectObject(hdc, green_pen);
			EndPaint(hwnd, &ps);
		}
		break;
		case WM_LBUTTONDOWN:
			//Captura posição do cursor e armazena na posição atual
			GetCursorPos(&control_points[curr_point]);

			if(is_inside_window(control_points[curr_point].x, control_points[curr_point].y)){
				curr_point++;
				InvalidateRect(hwnd, 0, TRUE);
			}else{
				control_points[curr_point].x = -1;
				control_points[curr_point].y = -1;
			}
		break;
		case WM_KEYDOWN:
			if(wParam == 'Z'){
				if(control_points[0].x >= 0 && curr_point != 0){
					control_points[curr_point-1].x = -1;
					control_points[curr_point-1].y = -1;
					curr_point--;
					InvalidateRect(hwnd, 0, TRUE);
				}
				if(curr_point == 0){
					control_points = (POINT*)realloc(control_points, 4*sizeof(POINT));
					control_points_size = 4;
					curr_point = 0;
				}
			}
			if(wParam == 'C'){
				draw_convex_hull = !draw_convex_hull;
				InvalidateRect(hwnd, 0, TRUE);
			}
			if(wParam == 'P'){
				draw_points = !draw_points;
				InvalidateRect(hwnd, 0, TRUE);
			}
		break;
		case WM_CLOSE:
			if(control_points[0].x >= 0){
				int id = MessageBox(hwnd, "Deseja Salvar?", "Salvar", MB_YESNOCANCEL);
				if(id == IDYES){
					SendMessage(hwnd, WM_COMMAND, ID_FILE_SAVE, 0);
				}else if(id == IDNO){
					free(control_points);
					DestroyWindow(hwnd);
				}
			}else{
				free(control_points);
				DestroyWindow(hwnd);
			}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc)){
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	printf("Digite o sistema de coordenadas desejado (i.e 800x600): ");
	scanf("%lfx%lf", &window_sizes[0], &window_sizes[1]);

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"Bezier Curves",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL){
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}


	ShowWindow(hwnd, SW_MAXIMIZE);
	GetWindowRect(hwnd, &viewport_rect);
	calculate_window(window_sizes[0], window_sizes[1]);
	UpdateWindow(hwnd);

	while(GetMessage(&Msg, NULL, 0, 0) > 0){
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
