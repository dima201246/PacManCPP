#include <iostream>
#include <string>
# ifdef WIN32
#include <windows.h>
#else
#include <stdlib.h>
# endif
#include <fstream>
using namespace std;

string long_string;

bool cut_help1(string line, int pos, int dlinaLine) // Функция проверки пробелов справа
{
	for (int i = pos; i <= dlinaLine; i++) {
		if (!line[i]) return false; // Если имеется знак конца строки
		if ((line[i] == '#') && (line[i - 1] != '/')) return false; // Если имеется комментарий, и он не экранирован
		if (line[i] != ' ') return true; // Если обнаружен знак отличный от пробела
	}
	return false;
}

bool long_line_test(string line) {
	for (unsigned i = 0; i <= line.length(); i++) {
		if (line[i] == '\"') {
			if (((i - 1) <= 0) && (line[i-1] == '/')) continue;
			else return true;
		}
	}
	return false;
}

string cut1(string line/* сам текст */, int par /*1 - вернуть текст до, 2 - вернуть текст после*/, char sign /*До или после какого знака вернуть текст*/) // CUT ver: 3.0
{
    string banka = ""; // Временная переменная
    int dlinaLine = line.length(); // Запись длины строчки
	bool ukaz = false; // Переменная для проверки знака препинания
    if (par == 1) { // вернуть текст до знака
        for (int i = 0; i <= dlinaLine; i++) {
			if (((i + 1) < dlinaLine) && (line[i] == ' ') && (line[i + 1] == ' ')) // Если много пробелов
				continue; // Если да, то пропуск
			if ((i + 2) <= dlinaLine) { // Если не достигнут конец строки и остается ещё два символа
				if ((line[i + 1] == ' ') && (line[i + 2] == sign)) { // Определение есть ли пробел и знак препинания после него
					banka = banka + line[i]; ukaz = true; break; } else // Если да, то остановка
				if ((line[i] == ' ') && (line[i + 1] == sign)) // Если текущий знак пробел, а следующий знак препинания
					{ ukaz = true; break; }; // Если да, то остановка
			}
			if (!(banka[0]) && (line[i] == ' ')) continue; // Если перед переменной пробел
			if (line[i] == ' ') return "0"; // Если в середине переменной найден пробел, вернуть 0
			if (line[i] != sign) // Если не знак препинания
				banka = banka + line[i]; // Помещене знака в переменную
			else { ukaz = true; break; } // Если знак препинания - остановиться
        }
    } else { // Вернуть текст после знака
		bool space = false; // Для проверки пробела перед значением
		int pos_sign = 0; // Позиция после знака препинания
		for (int i = 0; i <= dlinaLine; i++) // Определение позиции знака препинания
			if (line[i] == sign) { // Если это знак препинания
				ukaz = true; // Знак препинания найден
				pos_sign = i + 1; // Сдвиг на одну позицию вправо 
				break; } // Остановить, если всё найдено
		if ((ukaz == false) || (pos_sign == dlinaLine)) return "0"; // Вернуть 0, если после знака препинания пусто или знак препинания отсутствует
		for (int i = pos_sign; i <= dlinaLine; i++) { // Начало иследования после знака препинания
			if ((i == pos_sign) && (line[i] == ' ')) { // Если сразу после знакапрепинания пробел
				space = true; // Обнаружен пробел
				continue; // Пропуск пробела
			}
			if ((space == true) && (line[i] == ' ')) continue; // Пока не пройдут все пробелы после знака препинания пропуск
			else space = false; // Если пробелы закончились
			if (line[i] == ' ') { // Если обнаружен пробел в значении
				if (cut_help1(line, i, dlinaLine) == true) { // Если после пробелов имеется какойто знак отличный от пробела
					banka = banka + line[i]; // То добавить пробелы в переменную
					continue; } else break; } // Если после пробелов ничего, остановка
			if ((line[i] == '/') && (line[i + 1] == '#')) continue; // Для того чтобы не добавлялся знак экранирования
			if ((line[i] == '%') && (line[i - 1] == '/')) continue; // Для того чтобы не добавлялся экранированный знак пробела
			if ((line[i] == '#') && (line[i - 1] != '/')) break; // если встречен неэкранированный знак комментария, то остановка
			if ((line[i] == '/') && (line[i + 1] == '%')) { banka = banka + ' '; continue; }; // Если встречен знак экранирования, а после него знак пробела, поставить пробел в переменную
			if (line[i]) // Если имеется знак (от знака конца строки)
				banka = banka + line[i]; } // То записать в переменную
		if (!banka[0]) return "0"; // если переменная пуста, то вернуть 0
	}
	if (ukaz == false) return "0"; // если знака препинания не найдено, то вернуть 0
	else return banka; // Иначе вернуть значение переменной
}

void FileConf(string langFile) {
	FILE *fp;
	if ((fp = fopen(langFile.c_str(), "r")) == NULL){
		cout << "Not found configuration file!" << endl;
		system("pause");
		exit(-1);
	}
	fclose(fp);
}

string conf (string parametr, string linkConf) {
    FileConf(linkConf);
    string readText, banka1;
    ifstream i(linkConf.c_str());
    string banka, long_banka1;
    bool long_line = false;
    banka.clear();
    long_banka1.clear();
    banka1.clear();
    while(true) {
        getline(i, readText);
    	banka.clear();
        if (i) {
        	if ((long_line == true) && (long_line_test(readText) == false)) {
				long_banka1 = long_banka1 + readText;
        		continue;
        	} else if (long_line == true) {
        		long_line = false;
				long_banka1 = long_banka1 + readText;
        		banka1 = long_banka1;
        		break;
        	}
            if (cut1(readText, 1, '=') == parametr) {
            	banka = cut1(readText, 2, '=');
            	if (long_line_test(banka) == true) {
            		long_line = true;
            		banka.erase(0, 1);
            		long_banka1 = long_banka1 + banka;
            		continue;
            	} else banka1 = banka;
            }
        } else break;
	}
	i.close();
    if (banka1 == "") {
        cout << "Critical error! Not found parametr: " << parametr << endl;
        system("pause");
        exit(0);
    } else
    return banka1;
}
/*
int main () {
	cout << conf("test_bug", ".\\pac.map") << endl;
	system("pause");
	return 0;
}
*/
