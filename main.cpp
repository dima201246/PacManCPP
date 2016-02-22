/*
	:DVcompany Games (2015)
	"Pac-man in a bad code..."
	Я не знаю зачем я написал эту недо-игру, но точно знаю,
	что даже если я выложу этот код в открытый доступ, ИК модулей conf, edit_conf, lang_out
	никто и никогда не увидит, потому что я писал эти модули год назад и там сплошные костыли (исправлять всё желания нет)
*/

# ifdef WIN32
# define SLEEP_SYSTEM Sleep
# define SLEEP_MUL 1
# define linux_shift 0
# include <windows.h>
# else
# define linux_shift 1
# define SLEEP_SYSTEM usleep
# define SLEEP_MUL 1000
# include <unistd.h>
# include <stdlib.h>
# endif
# include <fstream>
# include <curses.h>
# include <sstream>

# ifdef WIN32
# define SCORE_FILE "score.pac"
# else
# define SCORE_FILE "score_linux.pac"
# endif

using namespace std;

const string ver = "v1.1.4";

extern string conf (string parametr, string linkConf);
extern void edit_conf(string linkToconf, string parametr, string new_value);

struct ghost_object {
	int posX, posY, direction, iseehim;
	bool tunnel_detected, out_of_tunnel;
};

int score = 0; // Очки
int dots = 0; // Кол-во точек
int map_size[2]; // Размер карты
int lives = -5; // -5 для загрузки жизней из карты
int pac_coord[2]; // Координаты Пак-мена
int god_mode_delay_count = 0; // Время ГодМоде
int god_mode_delay;
/*pac_coord
0 - x
1 - y*/
int pac_coord_spawn[2]; // Координаты спавна Пак-мена
int direction = 0; // Направление, 0 - не двигаться
int tp_coord[4]; // Координаты туннелей
int tp_direction[2]; // Направления выхода из туннелей
int num_of_ghosts; // Колличество призраков
int ghosts_home[2]; // Координаты дома приведений

int update_map = 0; // Кол-во обновлений экрана (нужно для подсчёта очков)
int keypressed = 0;

bool god_mode_on = false; // Режим ГодМоде
bool god_mode_always = false; // Чит режим
bool pac_open = true; // Открыт ли рот у Пак-мена
bool first_start = true; // Первое открытие 

# ifdef WIN32
string map_now = ".\\maps\\pac.map"; // Начальная карта
# else
string map_now = "./maps/pac.map"; // Начальная карта
# endif

string level; // Номер или название уровня
string debug; // Для отладки карт

void system_pause() { // Ожидание нажатия любой кнопки
	printw("Press any key...\n");
	getch();
}

int main();

string del_new_str (string line) { // Удаление символа новой строки (больше для Linux Based OS)
	string banka;
	banka.clear();
	for (unsigned i = 0; i < line.length(); i++)
		if ((int)line[i] != 13) banka += line[i]; // Если 13 символ, то не записывать в строку
	return banka;
}


void load_map(char **game_map) { // Загрузка карты в ОЗУ
	string map_line = del_new_str(conf("map", map_now));
	int k = 0;
	for (int y = 0; y <= map_size[1]; y++) // Заполнение карты пустотой
		for (int x = 0; x <= map_size[0]; x++)
			game_map[x][y] = 'e';
	for (int y = 1; y < map_size[1]; y++)
		for (int x = 1; x < map_size[0]; x++) {
			game_map[x][y] = map_line[k];
			k++;
		}
	return;
}

void write_map(char **game_map, ghost_object *ghosts, bool kill, char pac_skill) { // Вывод интерфейса
	update_map++; // Счётчик обновлений карты
	printw("Level: %s%s%i%s%i\n", level.c_str(), " Lives: ", lives, " Score: ", score); // Вывод шапки интерфейса
	int count_continue = 0; // Пропуск ходов для вывода призраков (честно говоря, не совсем понимаю как это работает)
	int old_ghost[2]; /*координата для предыдущего приведения*/ old_ghost[0] = 0; old_ghost[1] = 0;
	for (int y = 1; y < map_size[1]; y++) { // Работа с массивом
		for (int x = 1; x < map_size[0]; x++) {
			// count_continue = 0;
			if (!kill) { // Если пакмена не убили
				for (int i = 0; i < num_of_ghosts; i++) { // Обработка приведений
					if ((ghosts[i].posX == x) && (ghosts[i].posY == y) && ((old_ghost[0] != x) || (old_ghost[1] != y))) { // Если под новым приведением нет старого
						if (god_mode_on) attron (COLOR_PAIR(3) | A_BOLD); // Если активен ГодМоде, то приведения будут синего цвета
						else attron (COLOR_PAIR(4) | A_BOLD); // Иначе красного
						printw("G");
						if (god_mode_on) attroff (COLOR_PAIR(3) | A_BOLD);
						else attroff (COLOR_PAIR(4) | A_BOLD);
						count_continue++; // Счётчик кол-ва приведений, для пропуска (может это вообще не надо...)
						old_ghost[0] = x; old_ghost[1] = y;
					}
				}
				if (count_continue > 0) { // Пропуск загрузки других объектов
					count_continue--;
					continue;
				}
			}
			if ((pac_coord[0] == x) && (pac_coord[1] == y)) { // Если найден Пак-мен в массиве
				attron (COLOR_PAIR(1) | A_BOLD); // Жёлтый цвет
				if ((pac_open) && (!kill)) { // Если пакмен с открытым ртом и его не убили
					switch (direction) { // Рисование Пак-мена в зависимости от направления
						case 1: printw("v"); break; // Вверх
						case 2: printw("^"); break; // Вниз
						case 3: printw(">"); break; // Влево
						case 4: printw("<"); break; // Вправо
						default: printw(">"); break; // Если направление не задано
					}
					pac_open = false; // Закрыть рот Пак-мену
				} else if (!kill) { // Если рот закрыт и Пак-мен жив
					switch (direction) { // Рисование Пак-мена в зависимости от направления
						case 1: printw("|"); break; // Вверх
						case 2: printw("|"); break; // Вниз
						case 3: printw("-"); break; // Влево
						case 4: printw("-"); break; // Вправо
						default: printw("-"); break; // Если направление не задано
					}
					pac_open = true; // Открыть рот Пак-мену
				} else printw("%c", pac_skill); // Если Пак-мен убит, то вывести передаваемый символ
				attroff (COLOR_PAIR(1) | A_BOLD);
				continue; // Пропуск интерации, чтобы вместо Пак-мена не нарисовало что-нибудь другое
			}
			if (game_map[x][y] == 'w'){ // Вывод стены
				attron (COLOR_PAIR(2) | A_BOLD); // Белый цвет
				printw("#"); // Стена
				attroff (COLOR_PAIR(2) | A_BOLD);
			} else if (game_map[x][y] == 's') { // Вывод точки
				attron (COLOR_PAIR(5) | A_BOLD); // Зелёный цвет
				printw("."); // Обычная точка
				attroff (COLOR_PAIR(5) | A_BOLD);
			} else if (game_map[x][y] == 'e') printw(" "); // Пустота
			else if (game_map[x][y] == 'b') { // Вывод Божинки
				attron (COLOR_PAIR(1) | A_BOLD); // Жёлтый цвет
				printw("*"); // Божинка
				attroff (COLOR_PAIR(1) | A_BOLD);
			} else if (game_map[x][y] == 'd') printw("X"); // Дверь для приведений
			else printw(" "); // Если неизвестный сивол, то пустота
		}
		printw("\n"); // Спуск строки
	}
	if (debug == "1") { // Вывод координат приведений
		int i;
		for (i = 0; i < num_of_ghosts; i++)
			mvprintw(i + 1, map_size[0] + 1, "coord_x_%i%s%i%s%i%s%i", i, ": ", ghosts[i].posX, " coord_y_", i, ": ", ghosts[i].posY);
		mvprintw(i + 1, map_size[0] + 1, "pacman_x_%i%s%i%s%i%s%i", i, ": ", pac_coord[0], " pacman_y_", i, ": ", pac_coord[1]);
	}
}

void pause(char **game_map, ghost_object *ghosts) { // Пауза
	erase(); // Очистка экрана
	printw("Level: %s%s%i%s%i%s", level.c_str(), " Lives: ", lives, " Score: ", score, "\n\nPause\n\n");
	timeout(-1);
	system_pause(); // Ожидание нажатия любой кнопки
	erase();
	write_map(game_map, ghosts, false, 'p'); // Перерисовка интерфейса
	return;
}

bool freedom(int direction_local, char **game_map) { // Проверка свободы Пак-мена
	switch (direction_local) { // Свобода зависит от направления
		case 1: if ((game_map[pac_coord[0]][pac_coord[1] - 1] != 'w') && (game_map[pac_coord[0]][pac_coord[1] - 1] != 'd')) return true; else return false; break; // UP
		case 2: if ((game_map[pac_coord[0]][pac_coord[1] + 1] != 'w') && (game_map[pac_coord[0]][pac_coord[1] + 1] != 'd')) return true; else return false; break; // DOWN
		case 3: if ((game_map[pac_coord[0] - 1][pac_coord[1]] != 'w') && (game_map[pac_coord[0] - 1][pac_coord[1]] != 'd')) return true; else return false; break; // LEFT
		case 4: if ((game_map[pac_coord[0] + 1][pac_coord[1]] != 'w') && (game_map[pac_coord[0] + 1][pac_coord[1]] != 'd')) return true; else return false; break; // RIGHT
		default: return false; break; // В случае другого направления - ложь
	}
	return false;
}

bool ghost_search(ghost_object *ghosts, int direction_local, int coord_x, int coord_y) { // Поиск приведений рядом, дабы они не слипались
	switch (direction_local) { // Свобода зависит от направления
		case 1:
				for (int i = 0; i < num_of_ghosts; i++) {
					if (((coord_x) == ghosts[i].posX) && ((coord_y - 1) == ghosts[i].posY)) return false;
				}
				return true;
				break; // UP
		case 2:
				for (int i = 0; i < num_of_ghosts; i++) {
					if (((coord_x) == ghosts[i].posX) && ((coord_y + 1) == ghosts[i].posY)) return false;
				}
				return true;
				break; // DOWN
		case 3:
				for (int i = 0; i < num_of_ghosts; i++) {
					if (((coord_x - 1) == ghosts[i].posX) && ((coord_y) == ghosts[i].posY)) return false;
				}
				return true;
				break; // LEFT
		case 4:
				for (int i = 0; i < num_of_ghosts; i++) {
					if (((coord_x + 1) == ghosts[i].posX) && ((coord_y) == ghosts[i].posY)) return false;
				}
				return true;
				break; // RIGHT
	}
	return false;
}

bool freedom_ghosts(int direction_local, int coord_x, int coord_y, char **game_map, ghost_object *ghosts) { // Проверка свободы призраков
	switch (direction_local) { // Свобода зависит от направления
		case 1: if ((game_map[coord_x][coord_y - 1] != 'w') && (ghost_search(ghosts, 1, coord_x, coord_y))) return true; else return false; break; // UP
		case 2: if ((game_map[coord_x][coord_y + 1] != 'w') && (ghost_search(ghosts, 2, coord_x, coord_y))) return true; else return false; break; // DOWN
		case 3: if ((game_map[coord_x - 1][coord_y] != 'w') && (ghost_search(ghosts, 3, coord_x, coord_y))) return true; else return false; break; // LEFT
		case 4: if ((game_map[coord_x + 1][coord_y] != 'w') && (ghost_search(ghosts, 4, coord_x, coord_y))) return true; else return false; break; // RIGHT
		default: return false; break; // В случае другого направления - ложь
	}
	return false;
}

bool no_to_pac(int direction_local, char **game_map, ghost_object *ghosts, int num_ghost) { // Функция, чтобы не двигаться к Пак-мену, если он съел божинку
	int busy_way = 0; // Переменная для занятого пути
	if (!god_mode_on) return false; // Если не съедена божинка, то вернуть ложь
	switch (direction_local) { // Если по выбранному направлению есть Пак-мен
		case 1: if ((pac_coord[0] == ghosts[num_ghost].posX) && (pac_coord[1] < ghosts[num_ghost].posY)) busy_way = 1; break;
		case 2: if ((pac_coord[0] == ghosts[num_ghost].posX) && (pac_coord[1] > ghosts[num_ghost].posY)) busy_way = 2; break;
		case 3: if ((pac_coord[1] == ghosts[num_ghost].posY) && (pac_coord[0] < ghosts[num_ghost].posX)) busy_way = 3; break;
		case 4: if ((pac_coord[1] == ghosts[num_ghost].posY) && (pac_coord[0] > ghosts[num_ghost].posX)) busy_way = 4; break;
	}
	if (busy_way == 0) return false; // Если выбранное направление не занято Пак-меном
	bool find_free_way = false;
	if ((busy_way != 1) && (freedom_ghosts(1, ghosts[num_ghost].posX, ghosts[num_ghost].posY, game_map, ghosts))) find_free_way = true;
	if ((busy_way != 2) && (freedom_ghosts(2, ghosts[num_ghost].posX, ghosts[num_ghost].posY, game_map, ghosts))) find_free_way = true;
	if ((busy_way != 3) && (freedom_ghosts(3, ghosts[num_ghost].posX, ghosts[num_ghost].posY, game_map, ghosts))) find_free_way = true;
	if ((busy_way != 4) && (freedom_ghosts(4, ghosts[num_ghost].posX, ghosts[num_ghost].posY, game_map, ghosts))) find_free_way = true;
	if (find_free_way) return true; // Если был найден альтернативный путь, вернуть правду для продолжения цикла
	else return false; // Иначе смириться и ждать своей учести...
}

void action_ghost(int direction_local, char **game_map, ghost_object *ghosts, int num_ghost) { // Рандомное движение приведений (по ходу дела, это, считай, мозги приведений!)
	if ((ghosts[num_ghost].iseehim != 0) && (freedom_ghosts(ghosts[num_ghost].iseehim, ghosts[num_ghost].posX, ghosts[num_ghost].posY, game_map, ghosts))) { // Если приведение ранее увидело Пак-мена и если по выбранному направлению нет стены
		switch (direction_local) {
			case 1: ghosts[num_ghost].posY--; return; break; // Вверх
			case 2: ghosts[num_ghost].posY++; return; break; // Вниз
			case 3: ghosts[num_ghost].posX--; return; break; // Влево
			case 4: ghosts[num_ghost].posX++; return; break; // Вправо
		}
	}
	ghosts[num_ghost].iseehim = 0; // Сброс направления
	int bad_way = 0;
	switch (direction_local) { // Чтобы эти дибилы не бежали назад
		case 1: bad_way = 2; break; // Вверх
		case 2: bad_way = 1; break; // Вверх
		case 3: bad_way = 4; break; // Вверх
		case 4: bad_way = 3; break; // Вверх
	}
	int rand_num = rand() % 4 + 1; // Рандомное число от 1 до 4
	while (rand_num == bad_way)
		rand_num = rand() % 4 + 1; // Рандомное число от 1 до 4
	bool get_value[4];
	for (int i = 0; i < 4; i++)
		get_value[i] = false;
	get_value[bad_way-1] = true;
	while ((!freedom_ghosts(rand_num, ghosts[num_ghost].posX, ghosts[num_ghost].posY, game_map, ghosts)) || (no_to_pac(rand_num, game_map, ghosts, num_ghost))) { // Если по рандомному направлению нет стены или по выбранному направлению нет Пак-мена (если съедена Божинка)
		get_value[rand_num - 1] = true;
		// Cycle cycle motocycle!
		if ((get_value[0]) && (get_value[1]) && (get_value[2]) && (get_value[3])) {
			rand_num = 0;
			break;
		}
		rand_num = rand() % 4 + 1; // Новое рандомное направление
		while (rand_num == bad_way)
			rand_num = rand() % 4 + 1; // Новое рандомное направление
	}
	ghosts[num_ghost].direction = rand_num; // Задание нового направления
	switch (rand_num) { // Движение по выбранному направлению
		case 1: ghosts[num_ghost].posY--; break; // Вверх
		case 2: ghosts[num_ghost].posY++; break; // Вниз
		case 3: ghosts[num_ghost].posX--; break; // Влево
		case 4: ghosts[num_ghost].posX++; break; // Вправо
	}
	return;
}

void ghosts_ii(char **game_map, ghost_object *ghosts) { // Движение приведений за Пак-меном
	int non_action = 0; // Если не будет на пути Пак-мена, то рандомное движение
	if (!god_mode_on) { // Если Пак-мен не съел Божинку
		for (int i = 0; i < num_of_ghosts; i++) {
			non_action = 0; // Если не будет на пути Пак-мена, то рандомное движение

			if ((ghosts[i].iseehim != 0) && (ghosts[i].tunnel_detected)) { // Следование за Пак-меном в туннель
				ghosts[i].out_of_tunnel = true; // Если Пак-мен зашёл в туннель
				switch (ghosts[i].iseehim) {
					case 1: if (freedom_ghosts(1, ghosts[i].posX, ghosts[i].posY, game_map, ghosts)) {ghosts[i].posY--; continue;} else {ghosts[i].out_of_tunnel = false; ghosts[i].out_of_tunnel = false; ghosts[i].direction = 0;} break;
					case 2: if (freedom_ghosts(2, ghosts[i].posX, ghosts[i].posY, game_map, ghosts)) {ghosts[i].posY++; continue;} else {ghosts[i].out_of_tunnel = false; ghosts[i].out_of_tunnel = false; ghosts[i].direction = 0;} break;
					case 3: if (freedom_ghosts(3, ghosts[i].posX, ghosts[i].posY, game_map, ghosts)) {ghosts[i].posX--; continue;} else {ghosts[i].out_of_tunnel = false; ghosts[i].out_of_tunnel = false; ghosts[i].direction = 0;} break;
					case 4: if (freedom_ghosts(4, ghosts[i].posX, ghosts[i].posY, game_map, ghosts)) {ghosts[i].posX++; continue;} else {ghosts[i].out_of_tunnel = false; ghosts[i].out_of_tunnel = false; ghosts[i].direction = 0;} break;
				}
			}
			if ((pac_coord[0] == ghosts[i].posX) && (pac_coord[1] < ghosts[i].posY) && (freedom_ghosts(1, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 1; ghosts[i].posY--; ghosts[i].iseehim = 1; continue;} else non_action++; // Поиск и движение к Пак-мену, если он сверху
			if ((pac_coord[0] == ghosts[i].posX) && (pac_coord[1] > ghosts[i].posY) && (freedom_ghosts(2, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 2; ghosts[i].posY++; ghosts[i].iseehim = 2; continue;} else non_action++; // Поиск и движение к Пак-мену, если он снизу
			if ((pac_coord[1] == ghosts[i].posY) && (pac_coord[0] < ghosts[i].posX) && (freedom_ghosts(3, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 3; ghosts[i].posX--; ghosts[i].iseehim = 3; continue;} else non_action++; // Поиск и движение к Пак-мену, если он слева
			if ((pac_coord[1] == ghosts[i].posY) && (pac_coord[0] > ghosts[i].posX) && (freedom_ghosts(4, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 4; ghosts[i].posX++; ghosts[i].iseehim = 4; continue;} else non_action++; // Поиск и движение к Пак-мену, если он справа
			if ((ghosts[i].out_of_tunnel) && (ghosts[i].iseehim != 0)) {
				ghosts[i].direction = 0;
				ghosts[i].out_of_tunnel = false;
			} else ghosts[i].out_of_tunnel = false;
			if (non_action == 4) action_ghost(ghosts[i].direction, game_map, ghosts, i); // Рандомное движение приведений
		}
	} else {
		for (int i = 0; i < num_of_ghosts; i++) {
			non_action = 0; // Если не будет на пути Пак-мена, то рандомное движение
			if ((pac_coord[0] == ghosts[i].posX) && (pac_coord[1] > ghosts[i].posY) && (freedom_ghosts(1, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 1; ghosts[i].iseehim = 0; ghosts[i].posY--; continue;} else non_action++; // Поиск и убегание от Пак-мена, если он сверху
			if ((pac_coord[0] == ghosts[i].posX) && (pac_coord[1] < ghosts[i].posY) && (freedom_ghosts(2, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 2; ghosts[i].iseehim = 0; ghosts[i].posY++; continue;} else non_action++; // Поиск и убегание от Пак-мена, если он снизу
			if ((pac_coord[1] == ghosts[i].posY) && (pac_coord[0] > ghosts[i].posX) && (freedom_ghosts(3, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 3; ghosts[i].iseehim = 0; ghosts[i].posX--; continue;} else non_action++; // Поиск и убегание от Пак-мена, если он слева
			if ((pac_coord[1] == ghosts[i].posY) && (pac_coord[0] < ghosts[i].posX) && (freedom_ghosts(4, ghosts[i].posX, ghosts[i].posY, game_map, ghosts))) {ghosts[i].direction = 4; ghosts[i].iseehim = 0; ghosts[i].posX++; continue;} else non_action++; // Поиск и убегание от Пак-мена, если он справа
			if (non_action == 4) action_ghost(ghosts[i].direction, game_map, ghosts, i);  // Рандомное движение приведений
		}
	}
	bool tp_process = false;
	for (int i = 0; i < num_of_ghosts; i++) {
		tp_process = false;
		if ((ghosts[i].posX == tp_coord[2]) && (ghosts[i].posY == tp_coord[3])) { // Если совпадают координаты приведения со 2-м туннелем
			ghosts[i].tunnel_detected = false;

			if ((ghosts[i].direction == 3) || (ghosts[i].direction == 4)) { // Если это горизонтальное движение
				if ((map_size[0] + 1) <= (tp_coord[0] + 1)) // Если координата выходит за пределы карты
					ghosts[i].posX = tp_coord[0] - 1; // Перемещение Пак-мена к 1-му туннелю
				else
					ghosts[i].posX = tp_coord[0] + 1; // Перемещение Пак-мена к 1-му туннелю
			} else
				ghosts[i].posX = tp_coord[0]; // Перемещение Пак-мена к 1-му туннелю
			if ((ghosts[i].direction == 1) || (ghosts[i].direction == 2)) { // Если это вертикальное движение
				if (0 > (tp_coord[1] - 1)) // Если координата выходит за пределы карты
					ghosts[i].posY = tp_coord[1] + 1; // Перемещение Пак-мена к 1-му туннелю
				else
					ghosts[i].posY = tp_coord[1] - 1; // Перемещение Пак-мена к 1-му туннелю
			} else
				ghosts[i].posY = tp_coord[1]; // Перемещение Пак-мена к 1-му туннелю
			ghosts[i].direction = tp_direction[0]; // Изменение направления
			tp_process = true;
		}
		if ((ghosts[i].posX == tp_coord[0]) && (ghosts[i].posY == tp_coord[1]) && (tp_process == false)) { // Если совпадают координаты приведения с 1-м туннелем и вход во второй туннель не был выполнен
			ghosts[i].tunnel_detected = false;
			if ((ghosts[i].direction == 3) || (ghosts[i].direction == 4)) { // Если это горизонтальное движение
				if (0 > (tp_coord[2] - 1)) // Если координата выходит за пределы нуля, либо равна ему
					ghosts[i].posX = tp_coord[2] + 1; // Перемещение Пак-мена ко 2-му туннелю
				else
					ghosts[i].posX = tp_coord[2] - 1; // Перемещение Пак-мена ко 2-му туннелю
			}
			else
				ghosts[i].posX = tp_coord[2];  // Перемещение Пак-мена ко 2-му туннелю
			if ((ghosts[i].direction == 1) || (ghosts[i].direction == 2)) { // Если это вертикальное движение
				if ((map_size[1] + 1) <= (tp_coord[3] + 1)) // Если координата выходит за пределы карты
					ghosts[i].posY = tp_coord[3] - 1;  // Перемещение Пак-мена ко 2-му туннелю
				else
					ghosts[i].posY = tp_coord[3] + 1;  // Перемещение Пак-мена ко 2-му туннелю
			} else
				ghosts[i].posY = tp_coord[3];  // Перемещение Пак-мена ко 2-му туннелю
			ghosts[i].direction = tp_direction[1]; // Изменение направления
		}
	}
	return;
}

void ghosts_load(ghost_object *ghosts) { // Загрузка приведений в массив
	for (int i = 0; i < num_of_ghosts; i++) {
		ghosts[i].posX = ghosts_home[0];
		ghosts[i].posY = ghosts_home[1];
		ghosts[i].direction = rand() % 4 + 1; // Рандомное начальное направление движения
		ghosts[i].tunnel_detected = false;
	}
}

void dots_options(char **game_map) { // Операция проверки точек под Пак-меном
	if (game_map[pac_coord[0]][pac_coord[1]] == 's') { // Если обычная точка
		score += 10; // Добавить 10 очков
		// Beep(50, 750);
		++dots; // Счётчик точек
		game_map[pac_coord[0]][pac_coord[1]] = 'e'; // Замена точки на пустоту
	}
	if (game_map[pac_coord[0]][pac_coord[1]] == 'b') { // Если божинка
		score += 10; // Добавить 10 очков
		// Beep(50, 750);
		++dots; // Счетчик точек
		game_map[pac_coord[0]][pac_coord[1]] = 'e'; // Замена Божинки на пустоту
		if (god_mode_on) god_mode_delay_count -= god_mode_delay; // При повторном взятии Божинки время добовляется
		god_mode_on = true; // Включение временного бессмертия
	}
}

bool ghosts_direction(ghost_object *ghosts, int key_dir) { // проверка соприкосновения с приведениями Пак-мена
	for (int i = 0; i < num_of_ghosts; i++)
		if ((ghosts[i].posX == pac_coord[0]) && (ghosts[i].posY == pac_coord[1]) && (!god_mode_on) && (!god_mode_always)) { // Если новое направление не противоположно направлению приведения, то приведение Пак-мена не съест
			switch (ghosts[i].direction) {
				case 1: if (key_dir == 2) return true; else return false; break;
				case 2: if (key_dir == 1) return true; else return false; break;
				case 3: if (key_dir == 4) return true; else return false; break;
				case 4: if (key_dir == 3) return true; else return false; break;
			}
		}
	return true;
}

void key_pressed(char **game_map, ghost_object *ghosts, int keypressed);

void animation_dead(char **game_map, ghost_object *ghosts) { // Анимация смерти Пак-мена
	int last_count = 0;
	while (last_count < 4) {
		erase();
		switch (last_count) {
			case 0: switch (direction) {
						case 1: write_map(game_map, ghosts, true, 'v'); break;
						case 2: write_map(game_map, ghosts, true, '^'); break;
						case 3: write_map(game_map, ghosts, true, '>'); break;
						case 4: write_map(game_map, ghosts, true, '<'); break;
						default: write_map(game_map, ghosts, true, '>'); break;
					} break;
			case 1: write_map(game_map, ghosts, true, '*'); break;
			case 2: write_map(game_map, ghosts, true, '.'); break;
			case 3: write_map(game_map, ghosts, true, '+'); break;
		}
		timeout(0);
		getch();
		SLEEP_SYSTEM(400*SLEEP_MUL);
		last_count++;
	}
	return;
}

bool ghosts_killer(char **game_map, ghost_object *ghosts) { // Убийство или преведений или Пак-мена
	for (int i = 0; i < num_of_ghosts; i++) {
		if ((ghosts[i].posX == pac_coord[0]) && (ghosts[i].posY == pac_coord[1]) && (!god_mode_on) && (!god_mode_always)) {
			erase();
			animation_dead(game_map, ghosts);
			lives--;
			pac_coord[0] = pac_coord_spawn[0];
			pac_coord[1] = pac_coord_spawn[1];
			direction = 0;
			for (int i = 0; i < num_of_ghosts; i++) {
				ghosts[i].posX = ghosts_home[0];
				ghosts[i].posY = ghosts_home[1];
			}
			pac_open = true;
			timeout(-1);
			if (getch() == 27) key_pressed(game_map, ghosts, 27);
			return true;
			break;
		}
		if ((ghosts[i].posX == pac_coord[0]) && (ghosts[i].posY == pac_coord[1]) && (god_mode_on)) {
			score += 100;
			ghosts[i].posX = ghosts_home[0];
			ghosts[i].posY = ghosts_home[1];
			return true;
			break;
		}
	}
	return false;
}

void action_pac(char **game_map, ghost_object *ghosts) { // Автоматическое движение Пак-мена по направлению
	if (ghosts_direction(ghosts, direction)) // Если пользователь собирается резко убежать от приведения, то вернется ложь
		if ((ghosts_killer(game_map, ghosts)) && (god_mode_on)) return; // Действие при контакте с приведениями
	switch (direction) { 
		case 1: if (freedom(1, game_map)) --pac_coord[1]; break; // Проверка возможности движения вверх
		case 2: if (freedom(2, game_map)) ++pac_coord[1]; break; // Проверка возможности движения вниз
		case 3: if (freedom(3, game_map)) --pac_coord[0]; break; // Проверка возможности движения влево
		case 4: if (freedom(4, game_map)) ++pac_coord[0]; break; // Проверка возможности движения вправо
		default: break; // Иначе стоять на месте
	}
	dots_options(game_map); // Проверка наличия точек под Пак-меном
	if ((ghosts_killer(game_map, ghosts)) && (god_mode_on)) return; // Действие при контакте с приведениями
	dots_options(game_map); // Проверка наличия точек под Пак-меном
	bool tp_process = false; // Для того, чтобы Пак-мен не вошел в туннель из которого только что вышел
	if ((pac_coord[0] == tp_coord[2]) && (pac_coord[1] == tp_coord[3])) { // Если совпадают координаты Пак-мена со 2-м туннелем
		for (int i = 0; i < num_of_ghosts; i++)
			if (ghosts[i].iseehim != 0)
				ghosts[i].tunnel_detected = true;
		if ((direction == 3) || (direction == 4)) { // Если это горизонтальное движение
			if ((map_size[0] + 1) <= (tp_coord[0] + 1)) // Если координата выходит за пределы карты
				pac_coord[0] = tp_coord[0] - 1; // Перемещение Пак-мена к 1-му туннелю
			else
				pac_coord[0] = tp_coord[0] + 1; // Перемещение Пак-мена к 1-му туннелю
		} else
			pac_coord[0] = tp_coord[0]; // Перемещение Пак-мена к 1-му туннелю
		if ((direction == 1) || (direction == 2)) { // Если это вертикальное движение
			if (0 > (tp_coord[1] - 1)) // Если координата выходит за пределы карты
				pac_coord[1] = tp_coord[1] + 1; // Перемещение Пак-мена к 1-му туннелю
			else
				pac_coord[1] = tp_coord[1] - 1; // Перемещение Пак-мена к 1-му туннелю
		} else
			pac_coord[1] = tp_coord[1]; // Перемещение Пак-мена к 1-му туннелю
		direction = tp_direction[0]; // Изменение направления
		tp_process = true;
	}
	if ((pac_coord[0] == tp_coord[0]) && (pac_coord[1] == tp_coord[1]) && (!tp_process)) { // Если совпадают координаты Пак-мена с 1-м туннелем и вход во второй туннель не был выполнен
		for (int i = 0; i < num_of_ghosts; i++)
		if (ghosts[i].iseehim != 0)
			ghosts[i].tunnel_detected = true;
		if ((direction == 3) || (direction == 4)) { // Если это горизонтальное движение
			if (0 > (tp_coord[2] - 1)) // Если координата выходит за пределы нуля, либо равна ему
				pac_coord[0] = tp_coord[2] + 1; // Перемещение Пак-мена ко 2-му туннелю
			else
				pac_coord[0] = tp_coord[2] - 1; // Перемещение Пак-мена ко 2-му туннелю
		}
		else
			pac_coord[0] = tp_coord[2];  // Перемещение Пак-мена ко 2-му туннелю
		if ((direction == 1) || (direction == 2)) { // Если это вертикальное движение
			if ((map_size[1] + 1) <= (tp_coord[3] + 1)) // Если координата выходит за пределы карты
				pac_coord[1] = tp_coord[3] - 1;  // Перемещение Пак-мена ко 2-му туннелю
			else
				pac_coord[1] = tp_coord[3] + 1;  // Перемещение Пак-мена ко 2-му туннелю
		} else
			pac_coord[1] = tp_coord[3];  // Перемещение Пак-мена ко 2-му туннелю
		direction = tp_direction[1]; // Изменение направления
	}
	dots_options(game_map); // Проверка наличия точек под Пак-меном
	return;
}

void key_pressed(char **game_map, ghost_object *ghosts, int keypressed) { // Движение Пак-мена по по нажатию кнопки
	if (ghosts_direction(ghosts, keypressed)) // Если пользователь собирается резко убежать от приведения, то вернется ложь
		if ((ghosts_killer(game_map, ghosts)) && (god_mode_on)) return; // Действие при контакте с приведениями
	// keypressed = getch();
	// if (keypressed == 224) keypressed = getch(); // Защита от двойного символа стрелок
	switch (keypressed) {
		case 1: if (freedom(1, game_map)) {--pac_coord[1]; direction = 1;} else action_pac(game_map, ghosts); break; // UP
		case 2: if (freedom(2, game_map)) {++pac_coord[1]; direction = 2;} else action_pac(game_map, ghosts); break; // DOWN
		case 3: if (freedom(3, game_map)) {--pac_coord[0]; direction = 3;} else action_pac(game_map, ghosts); break; // LEFT
		case 4: if (freedom(4, game_map)) {++pac_coord[0]; direction = 4;} else action_pac(game_map, ghosts); break; // RIGHT
		case 13: pause(game_map, ghosts); break;
		case 27:
				erase(); // Очистка экрана
				printw("Are you sure? (enter = yes/any key = no)\n");
				if (getch() == '\n') { // Если пользователь хочет выйти
					for (int i = 0; i < map_size[0] + 1; ++i) // Очистка массива карты (хороший тон и предупреждение глюков винды)
						delete [] game_map[i];
					delete [] ghosts; // Удаление массива призраков
					main(); // Выход в меню
				}
				erase(); // Очистка экрана
				write_map(game_map, ghosts, false, 'p'); // Перерисовка интерфейса
				break; // EXIT
		default: action_pac(game_map, ghosts); break;
	}
	dots_options(game_map); // Проверка наличия точек под Пак-меном
	if ((ghosts_killer(game_map, ghosts)) && (god_mode_on)) return; // Действие при контакте с приведениями
	dots_options(game_map); // Проверка наличия точек под Пак-меном
	bool tp_process = false; // Для того, чтобы Пак-мен не вошел в туннель из которого только что вышел
	if ((pac_coord[0] == tp_coord[2]) && (pac_coord[1] == tp_coord[3])) { // Если совпадают координаты Пак-мена со 2-м туннелем
		for (int i = 0; i < num_of_ghosts; i++)
		if (ghosts[i].iseehim != 0)
			ghosts[i].tunnel_detected = true;
		if ((direction == 3) || (direction == 4)) { // Если это горизонтальное движение
			if ((map_size[0] + 1) <= (tp_coord[0] + 1)) // Если координата выходит за пределы карты
				pac_coord[0] = tp_coord[0] - 1; // Перемещение Пак-мена к 1-му туннелю
			else
				pac_coord[0] = tp_coord[0] + 1; // Перемещение Пак-мена к 1-му туннелю
		} else
			pac_coord[0] = tp_coord[0]; // Перемещение Пак-мена к 1-му туннелю
		if ((direction == 1) || (direction == 2)) { // Если это вертикальное движение
			if (0 > (tp_coord[1] - 1)) // Если координата выходит за пределы карты
				pac_coord[1] = tp_coord[1] + 1; // Перемещение Пак-мена к 1-му туннелю
			else
				pac_coord[1] = tp_coord[1] - 1; // Перемещение Пак-мена к 1-му туннелю
		} else
			pac_coord[1] = tp_coord[1]; // Перемещение Пак-мена к 1-му туннелю
		direction = tp_direction[0]; // Изменение направления
		tp_process = true;
	}
	if ((pac_coord[0] == tp_coord[0]) && (pac_coord[1] == tp_coord[1]) && (tp_process == false)) { // Если совпадают координаты Пак-мена с 1-м туннелем и вход во второй туннель не был выполнен
		for (int i = 0; i < num_of_ghosts; i++)
		if (ghosts[i].iseehim != 0)
			ghosts[i].tunnel_detected = true;
		if ((direction == 3) || (direction == 4)) { // Если это горизонтальное движение
			if (0 > (tp_coord[2] - 1)) // Если координата выходит за пределы нуля, либо равна ему
				pac_coord[0] = tp_coord[2] + 1; // Перемещение Пак-мена ко 2-му туннелю
			else
				pac_coord[0] = tp_coord[2] - 1; // Перемещение Пак-мена ко 2-му туннелю
		}
		else
			pac_coord[0] = tp_coord[2];  // Перемещение Пак-мена ко 2-му туннелю
		if ((direction == 1) || (direction == 2)) { // Если это вертикальное движение
			if ((map_size[1] + 1) <= (tp_coord[3] + 1)) // Если координата выходит за пределы карты
				pac_coord[1] = tp_coord[3] - 1;  // Перемещение Пак-мена ко 2-му туннелю
			else
				pac_coord[1] = tp_coord[3] + 1;  // Перемещение Пак-мена ко 2-му туннелю
		} else
			pac_coord[1] = tp_coord[3];  // Перемещение Пак-мена ко 2-му туннелю
		direction = tp_direction[1]; // Изменение направления
	}
	dots_options(game_map); // Проверка наличия точек под Пак-меном
	// while (kbhit()) // Если нажато слишком много кнопок
		// getch(); // Обработать их в никуда
	return;
}

string str(double input) {
	stringstream ss;
	ss << "";
	ss << input;
	string str;
	ss >> str;
	return str;
}

string name_winner(int score_local) {
	erase();
	string returned_value;
	returned_value.clear();
	int elem_selected = 1;
	/*95 == '_'
	65 == A
	90 == Z
	48 == 0
	57 == 9*/
	char elem1 = '_', elem2 = '_', elem3 = '_';
	string elem_s;
	int key_pressed_menu;
	while (true) {
		erase();
		attron (COLOR_PAIR(2) | A_BOLD);
		printw("New high score: %i\n%s\n", score_local, "Enter your nick:");
		attroff (COLOR_PAIR(2) | A_BOLD);
		if (elem_selected == 1) {
			attron (COLOR_PAIR(1) | A_BOLD);
			elem_s = elem1;
			printw(elem_s.c_str());
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			elem_s = elem1;
			printw(elem_s.c_str());
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (elem_selected == 2) {
			attron (COLOR_PAIR(1) | A_BOLD);
			elem_s = elem2;
			printw(elem_s.c_str());
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			elem_s = elem2;
			printw(elem_s.c_str());
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (elem_selected == 3) {
			attron (COLOR_PAIR(1) | A_BOLD);
			elem_s = elem3;
			printw(elem_s.c_str());
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			elem_s = elem3;
			printw(elem_s.c_str());
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		attron (COLOR_PAIR(2) | A_BOLD);
		printw(" - %i\n", score_local);
		attroff (COLOR_PAIR(2) | A_BOLD);
		if (elem_selected == 1) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("^");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" ");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (elem_selected == 2) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("^");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" ");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (elem_selected == 3) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("^");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" ");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		key_pressed_menu = getch();
		switch (key_pressed_menu) {
			case KEY_UP:
					switch (elem_selected) {
						case 1: if (((int)elem1 - 1) == 47) elem1 = '_';
								else if (((int)elem1 - 1) == 64) elem1 = '9';
								else if (((int)elem1 - 1) == 94) elem1 = 'Z';
								else elem1--;
								break;
						case 2: if (((int)elem2 - 1) == 47) elem2 = '_';
								else if (((int)elem2 - 1) == 64) elem2 = '9';
								else if (((int)elem2 - 1) == 94) elem2 = 'Z';
								else elem2--;
								break;
						case 3: if (((int)elem3 - 1) == 47) elem3 = '_';
								else if (((int)elem3 - 1) == 64) elem3 = '9';
								else if (((int)elem3 - 1) == 94) elem3 = 'Z';
								else elem3--;
								break;
					} break;
			case KEY_DOWN:
					switch (elem_selected) {
						case 1: if (((int)elem1 + 1) == 96) elem1 = '0';
								else if (((int)elem1 + 1) == 58) elem1 = 'A';
								else if (((int)elem1 + 1) == 91) elem1 = '_';
								else elem1++;
								break;
						case 2: if (((int)elem2 + 1) == 96) elem2 = '0';
								else if (((int)elem2 + 1) == 58) elem2 = 'A';
								else if (((int)elem2 + 1) == 91) elem2 = '_';
								else elem2++;
								break;
						case 3: if (((int)elem3 + 1) == 96) elem3 = '0';
								else if (((int)elem3 + 1) == 58) elem3 = 'A';
								else if (((int)elem3 + 1) == 91) elem3 = '_';
								else elem3++;
								break;
					} break;
			case 27: return "0"; break;
			case '\n':
					returned_value = (char)elem1;
					returned_value = returned_value + (char)elem2;
					returned_value = returned_value + (char)elem3;
					return returned_value;
					break;
			case KEY_LEFT: if ((elem_selected - 1) == 0) elem_selected = 3; else elem_selected--; break;
			case KEY_RIGHT: if ((elem_selected + 1) >= 4) elem_selected = 1; else elem_selected++; break;
		}
	}
}


string division_score(string line) { // Возврат только очков предыдущих игроков
	bool read = false; // Не читать
	string banka;
	banka.clear();
	for (unsigned i = 0; i < line.length(); i++) {
		if (line[i] == ';') { // Как только нашло точку с запятой - начать чтение
			read = true;
			continue;
		}
		if (read) banka = banka + line[i]; // Чтение
	}
	return banka;
}

void new_best_score(int score_local) {
	int mass_score[11];
	string banka_score, banka_score_with_name;
	erase();
	for (int i = 1; i <= 10; i++) {
		banka_score = "place_" + str(i);
		banka_score = conf(banka_score, SCORE_FILE);
		banka_score = division_score(banka_score);
		mass_score[i] = atoi(banka_score.c_str());
	}
	string name_winner_full;
	for (int i = 1; i <= 10; i++) {
		if (score_local > mass_score[i]) {
			banka_score = "place_" + str(i);
			name_winner_full = name_winner(score_local);
			if (name_winner_full == "0") {
				return;
				break;
			}
			for (int k = 10; k != i; k--) {
				banka_score = "place_" + str(k - 1);
				banka_score_with_name = conf(banka_score, SCORE_FILE);
				banka_score = "place_" + str(k);
				edit_conf(SCORE_FILE, banka_score, banka_score_with_name);
			}
			edit_conf(SCORE_FILE, "place_" + str(i), name_winner_full + ";" + str(score_local));
			break;
		}
	}
	// system_pause();
	return;
}

void game();

void win(char **game_map, ghost_object *ghosts) { // Если игрок выиграл
	erase(); // Очистка экрана
	int end_score = 10000 - update_map; // Высчитывание очков из колличества обновлений карты
	if (end_score < 0) end_score = score; // Если игрок икрал больше 100000 интераций, то защитать только очки
	else end_score += score; // Иначе очки + кол-во интераций не пройденных
	printw("Level: %s\n\n%s%i\n", level.c_str(), "You win!\n\nScore: ", end_score); // Вывод"You win!"
	string next_level = conf("next_level", map_now); // Загрузка номера следующего уровня
	if (next_level != "end") { // Если есть следующий уровень
			for (int i = 0; i < map_size[0] + 1; ++i) // Очистка массива карты
			 	delete [] game_map[i];
			delete [] ghosts; // Удаление массива призраков
		printw("Start level %s%s\n", next_level.c_str(), "? (enter = yes/esc = no)"); // Предложение начать следующий уровень
# ifdef WIN32
		map_now = ".\\maps\\" + conf("next_map", map_now); // Загрузка пути к новой карте
# else
		map_now = "./maps/" + conf("next_map", map_now); // Загрузка пути к новой карте
# endif
		dots = 0; // Онуление кол-ва точек
		update_map = 0; // Онуление кол-ва интераций
		direction = 0; // Онуление направления
		god_mode_always = false;
		god_mode_delay_count = 0;
		god_mode_on = false;
		pac_open = true;
		debug = "0";
		direction = 0;
		int cn;
		while (true) {
			cn = getch();
			if (cn == '\n') { // Если нажат ентер
				erase(); // Очистка экрана
				game(); // Начало новой игры
				break;
			} else if (cn == 27) {
				new_best_score(end_score);
				return;
			} // Выход
		}
	} else {
		new_best_score(end_score);
	}
	// system_pause(); // Если следующего уровня нет, то выход
	return;
}

void game_over(char **game_map, ghost_object *ghosts) { // Проигрыш
	erase(); // Очистка экрана
	int end_score = 10000 - update_map; // Высчитывание очков из колличества обновлений карты
	if (end_score < 0) end_score = score; // Если игрок икрал больше 100000 интераций, то защитать только очки
	else end_score += score; // Иначе очки + кол-во интераций не пройденных
	new_best_score(end_score);
	erase();
	printw("Level %s%s%i", level.c_str(), "\n\nYou dead!\n\nScore: ", end_score);
	for (int i = 0; i < map_size[0] + 1; ++i) // Очистка массива карты
	 	delete [] game_map[i];
	delete [] ghosts; // Удаление массива призраков
	printw("\nStart again? (enter = yes/esc = no)\n"); // Предложение начать играть заного
# ifdef WIN32
	map_now = ".\\maps\\pac.map"; // Подключение первого уровня
# else
	map_now = "./maps/pac.map"; // Подключение первого уровня
# endif
	dots = 0; // Онулениекол-ва точек
	lives = -5; // -5 жизней, чтобы они взялись из файла
	score = 0; // Онуленеи очков
	update_map = 0; // Онуление кол-ва интераций
	direction = 0; // Онуление направления
	god_mode_always = false;
	god_mode_delay_count = 0;
	god_mode_on = false;
	pac_open = true;
	debug = "0";
	int cn;
	while (true) {
		cn = getch();
		if (cn == '\n') {// Если нажат ентер
			erase(); // Очистка экрана, на всякий случай
			game(); // Запуск новой игры
			break;
		} else if (cn == 27) return; // Иначе выход
	}
	return;
}

void anti_out(ghost_object *ghosts) {
	for (int i = 0; i < num_of_ghosts; i++) {
		if ((ghosts[i].posX <= 0) || (ghosts[i].posY <= 0) || (ghosts[i].posX >= map_size[0]) || (ghosts[i].posY >= map_size[1])) {
			ghosts[i].posX = ghosts_home[0];
			ghosts[i].posY = ghosts_home[1];
		}
	}
	if ((pac_coord[0] <= 0) || (pac_coord[1] <= 0) || (pac_coord[0] >= map_size[0]) || (pac_coord[1] >= map_size[1])) {
		pac_coord[0] = pac_coord_spawn[0];
		pac_coord[1] = pac_coord_spawn[1];
		direction = 0;
	}
}

void game() {
	// Инициализация
	direction = 0;
	string banka; // Временная переменная
	banka.clear();
	level = conf("level", map_now); // Получение размера карты по X
	banka.clear();
	banka = conf("map_size_x", map_now); // Получение размера карты по X
	map_size[0] = atoi(banka.c_str()) + 1;
	banka.clear();
	banka = conf("map_size_y", map_now); // Получение размера карты по Y
	map_size[1] = atoi(banka.c_str()) + 1;
	banka.clear();
	int maxX, maxY;
	getmaxyx(stdscr, maxY, maxX);
	if ((maxX < map_size[0] + 1) || (maxY < map_size[1] + 1)) {
		erase();
		printw("Error! Screen small for this map!\n\n");
		system_pause();
		return;
	}
	banka = conf("speed", map_now); // Получение скорости, чем меньше, тем выше
	int DELAY_PAC = atoi(banka.c_str());
	banka.clear();
	if (lives == -5) {
		string banka = conf("lives", map_now); // Получение жизней
		lives = atoi(banka.c_str());
	}
	banka.clear();
	banka = conf("spawn_pac_x", map_now); // Получение начальных координат Пакмена, X
	pac_coord[0] = atoi(banka.c_str());
	pac_coord_spawn[0] = atoi(banka.c_str());
	banka.clear();
	banka = conf("spawn_pac_y", map_now); // Получение начальных координат Пак-мена, Y
	pac_coord[1] = atoi(banka.c_str());
	pac_coord_spawn[1] = atoi(banka.c_str());
	banka.clear();
	banka = conf("teleport1_x", map_now); // Получение координат 1-го туннеля, X
	tp_coord[0] = atoi(banka.c_str());
	banka.clear();
	banka = conf("teleport1_y", map_now); // Получение координат 1-го туннеля, Y
	tp_coord[1] = atoi(banka.c_str());
	banka.clear();
	banka = conf("teleport2_x", map_now); // Получение координат 2-го туннеля, X
	tp_coord[2] = atoi(banka.c_str());
	banka.clear();
	banka = conf("teleport2_y", map_now); // Получение координат 2-го туннеля, Y
	tp_coord[3] = atoi(banka.c_str());
	banka.clear();
	banka = conf("tp_direction_1", map_now); // Получение направления выхода 1-го туннеля
	tp_direction[0] = atoi(banka.c_str());
	banka.clear();
	banka = conf("tp_direction_2", map_now); // Получение направления выхода 2-го туннеля
	tp_direction[1] = atoi(banka.c_str());
	banka.clear();
	banka = conf("all_dots", map_now); // Получение направления выхода 2-го туннеля
	int all_dots = atoi(banka.c_str());
	banka.clear();
	banka = conf("ghost", map_now); // Получение направления выхода 2-го туннеля
	num_of_ghosts = atoi(banka.c_str());
	banka.clear();
	banka = conf("ghost_coord_home_x", map_now); // Получение начальных координат Пакмена, X
	ghosts_home[0] = atoi(banka.c_str());
	banka.clear();
	banka = conf("ghost_coord_home_y", map_now); // Получение начальных координат Пак-мена, Y
	ghosts_home[1] = atoi(banka.c_str());
	banka.clear();
	banka = conf("god_mode_delay", map_now); // Получение кол-ва интераций при котором действует Божинка
	god_mode_delay = atoi(banka.c_str());
	banka.clear();
	debug = conf("debug_map", map_now); // Получение направления выхода 2-го туннеля
	banka.clear();
	if (debug == "1") {
		banka = conf("god_mode_cheat", map_now); // Получение начальных координат Пак-мена, Y
		if (banka == "1") god_mode_always = true;
		else god_mode_always = false;
	}
	banka.clear(); // Инициализация конец
	char **game_map = new char* [map_size[0] + 1]; // Создание массива для карты (столбцы)
	for (int i = 0; i < map_size[0] + 1; ++i) // Строки
		game_map[i] = new char [map_size[1] + 1];
	load_map(game_map); // Загрузка карты в массив
	ghost_object *ghosts = new ghost_object [num_of_ghosts + 1]; // Создание массива для призраков
	ghosts_load(ghosts); // Загрузка приведений в массив
	erase(); // Очистка экрана
	system_pause(); // Ожидание любой кнопки
	god_mode_delay_count = 0; // Счетчик интераций с ГодМоде
	int cn, cn2;
	while (true) {
		erase();
		write_map(game_map, ghosts, false, 'p');
		if (god_mode_on) { // Если Пак-мен съел Божинку
			if (god_mode_delay_count > god_mode_delay) {god_mode_on = false; god_mode_delay_count = 0;} // Если счётчек дошёл до нужного числа, сбросить всё
			else god_mode_delay_count++; // Иначе ИНКРИМЕНТ!!!
		}
		ghosts_ii(game_map, ghosts); // Движение приведений
		timeout(0);
		cn = getch();
		cn2 = getch();
		while (cn2 != -1) { // Небольшой костыль, чтобы не обрабатывались лишние случайные кнопки
			if ((cn2 == KEY_UP) || (cn2 == KEY_DOWN) || (cn2 == KEY_LEFT) || (cn2 = KEY_RIGHT) || (cn2 == 27) || (cn2 == '\n')) {
				cn = cn2;
				break;
			}
			cn2 = getch();
		}
		switch (cn) {
			case KEY_UP: key_pressed(game_map, ghosts, 1); break;
			case KEY_DOWN: key_pressed(game_map, ghosts, 2); break;
			case KEY_LEFT: key_pressed(game_map, ghosts, 3); break;
			case KEY_RIGHT: key_pressed(game_map, ghosts, 4); break;
			case 27: timeout(-1); key_pressed(game_map, ghosts, 27); break;
			case '\n': key_pressed(game_map, ghosts, 13); break;
			default: action_pac(game_map, ghosts); break;
		}
		if (dots == all_dots) { // Если собраны все точки
			timeout(-1);
			win(game_map, ghosts); // Процедура выигрыша
			break; // Остановка цикла
		}
		if (lives < 0) { // Если закончились жизни
			timeout(-1);
			game_over(game_map, ghosts); // Процедура конца игры
			break; // Остановка цикла
		}
		anti_out(ghosts);
		SLEEP_SYSTEM(DELAY_PAC*SLEEP_MUL);
	}
}

void help() {
	erase();
	printw("Controls:\nArrow\n\nObjects:\nG - ghost (Kill you or you can kill their if you eat Godeat)\n> - Pac-man (It's you)\n. - eat (You will eat it for win)\n* - Godeat (If you eat it, you can kill ghosts)\n# - Wall\n\n");
	system_pause();
	return;
}

void information() {
	erase();
	printw("Pac-man in the a code...\n:DVcompany Games (2015)\nBadCoder: Dmitriy Volchenko\ne-mail: dima201246@gmail.com\nVersion: %s%s", ver.c_str(), "\n\n");
# ifdef WIN32
	printw("Windows compatibility\n\n");
# else
# 	ifdef linux
	printw("Linux Based OS compatibility\n\n");
# 	else
	printw("Unsupported OS\n\n");
# 	endif
# endif
	system_pause();
	return;
}

string edit_score_line(string line) {
	string banka; banka.clear();
	for (unsigned i = 0; i <= line.length(); i++) {
		if (line[i] == ';') {
			banka = banka + " - ";
			continue;
		}
		banka = banka + line[i];
	}
	return banka;
}

void best_score() {
	erase();
	for (int i = 1; i <= 10; i++)
		printw("%i%s%s%s", i - 1, ". ", edit_score_line(conf("place_" + str(i), SCORE_FILE)).c_str(), "\n");
	system_pause();
	return;
}

void title() {
	string line_creator = ":DVcompany ";
	string line_food = "ame... ";
	char letter_g = 'G';
	char pac_man_text = '>';
	char present_text[] = "Present";
	int maxX, maxY;
	getmaxyx(stdscr, maxY, maxX);
	int cn = 0, count = 0, prefix = 0;
	bool pac_open_local = true, break_stat = false;
	while (cn != 27) {
		if (!line_food.empty()) line_food.erase(line_food.length() - 1, 1);
		else {
			prefix = 1;
			switch (count) {
				case 0: pac_man_text = '>'; break;
				case 1: pac_man_text = '*'; break;
				case 2: pac_man_text = '.'; break;
				case 3: pac_man_text = '+'; break;
				case 4: break_stat = true; break;
			}
			count++;
		}
		if (break_stat) {
			attron (COLOR_PAIR(2) | A_BOLD);
			mvprintw((maxY / 2) + 2, (maxX - 25) / 2, "Pac-man in a bad code...");
			attroff (COLOR_PAIR(2) | A_BOLD);
			mvprintw((maxY / 2) + 4, (maxX - 13) / 2, "Press any key");
			mvprintw(/*maxY*/0, 0, "%s", ver.c_str());
			timeout(-1);
			getch();
			break;
		}
		erase();
		mvprintw((maxY / 2) - 2, (maxX - 19) / 2, "%s", line_creator.c_str());
		if (prefix == 0) {
			attron (COLOR_PAIR(4) | A_BOLD);
			mvprintw((maxY / 2) - 2, (maxX + 2 + linux_shift) / 2, "%c", letter_g);
			attroff (COLOR_PAIR(4) | A_BOLD);
		}
		mvprintw((maxY / 2) - 2, (maxX + 4 + linux_shift - prefix) / 2, "%s%c", line_food.c_str(), pac_man_text);
		mvprintw(maxY / 2, (maxX - sizeof(present_text)) / 2, "%s", present_text);
		if (pac_open_local) {
			pac_open_local = false;
			pac_man_text = '-';
		} else {
			pac_open_local = true;
			pac_man_text = '>';
		}
		timeout(0);
		cn = getch();
		SLEEP_SYSTEM(300*SLEEP_MUL);
	}
	timeout(-1);
	return;
}

void egg() { // Пасхальное яйцо
	erase();
	timeout(0);
	unsigned int pos_egg = 0;
	while (true) {
		erase();
		if (pos_egg == 0) {
			printw("0_o");
			pos_egg++;
		} else {
			pos_egg--;
			printw("o_0");
		}
		if (getch() == 27) break;
		SLEEP_SYSTEM(100*SLEEP_MUL);
	}
	timeout(-1);
	return;
}

int main() {
	setlocale(LC_ALL, "");
	initscr();
	start_color();
	keypad (stdscr, TRUE);
	noecho();
	curs_set(0);
	erase();
	init_pair (1, COLOR_YELLOW, COLOR_BLACK);
	init_pair (2, COLOR_WHITE, COLOR_BLACK);
	init_pair (3, COLOR_BLUE, COLOR_BLACK);
	init_pair (4, COLOR_RED, COLOR_BLACK);
	init_pair (5, COLOR_GREEN, COLOR_BLACK);
	if (first_start) {
		title();
		first_start = false;
	}
	FILE *fp;
	if ((fp = fopen(SCORE_FILE, "r")) == NULL){ // Если не найден файл с рекордами
		ofstream o(SCORE_FILE); // Открытие файла для записи
		for(int i = 1; i <= 10; i++) {
			o << "place_" << i << " = " << endl; // Запись
		}
		o.close(); // Закрыть файл для записи
	} else fclose(fp); // Закрыть файл
	int menu_set = 1, key_pressed_menu;
	while (true) {
		erase();
		if (menu_set == 1) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("> Start game\n");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" Start game\n");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (menu_set == 2) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("> Help\n");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" Help\n");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (menu_set == 3) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("> Table of results\n");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" Table of results\n");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (menu_set == 4) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("> Information\n");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" Information\n");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		if (menu_set == 5) {
			attron (COLOR_PAIR(1) | A_BOLD);
			printw("> Exit\n");
			attroff (COLOR_PAIR(1) | A_BOLD);
		} else {
			attron (COLOR_PAIR(2) | A_BOLD);
			printw(" Exit\n");
			attroff (COLOR_PAIR(2) | A_BOLD);
		}
		key_pressed_menu = getch();
		switch (key_pressed_menu) {
			case KEY_UP:
					if ((menu_set-1) <= 0) menu_set = 5;
					else menu_set--;
					break;
			case KEY_DOWN:
					if ((menu_set+1) > 5) menu_set = 1;
					else menu_set++;
					break;
			case 27: exit(0); break;
			case 101: egg(); break;
			case '\n':
					switch (menu_set) {
						case 1: game(); break;
						case 2: help(); break;
						case 3: best_score(); break;
						case 4: information(); break;
						case 5:
								erase();
								printw("Are you sure? (enter = yes/any key = no)\n");
								key_pressed_menu = getch();
								if (key_pressed_menu == '\n') {
									endwin();
									exit(0);
								}
								break;
					}
					break;
			default: break;
		}
	}
	endwin();
	return 0;
}
