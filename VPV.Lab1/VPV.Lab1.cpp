/////////////////////////////////////////////////////////////
// Log.cpp - файл отладки-демонстрации использования класса 
// протоколирования, обработки, вывода результатов измерений 
// Предполагается, что файл класса Log.h находится в материнском директории,
// а несколько использующих его приложений находятся в дочерних

#include <iostream>
#include <Windows.h>
#include <time.h>
#include "log.h"
#include <math.h>

using namespace std;

#define NS_IN_SEC 1.E9 // число наносекунд в секунде
#define MCS_IN_SEC 1.E6 // число микросекунд в секунде

// Макросы повторения для генерации продуктов разворачивания циклов
#define Repeat10(x) x x x x x x x x x x 
#define Repeat100(x) Repeat10(Repeat10(x))
#define Repeat1000(x) Repeat100(Repeat10(x))
#define Repeat10000(x) Repeat100(Repeat100(x))

// Длительность одного clock-интервала с помощью QPC
double сlockIntervalUsingQPC() {
	LARGE_INTEGER t_start, t_finish, freq;
	__int64 t_code;
	QueryPerformanceFrequency(&freq);
	clock_t t_clock = clock();
	while (clock() < t_clock + 1);
	QueryPerformanceCounter(&t_start);
	while (clock() < t_clock + 2);
	QueryPerformanceCounter(&t_finish);
	t_code = t_finish.QuadPart - t_start.QuadPart;
	return double(t_code) / freq.QuadPart;
};

// Длительность одного сlock-интервала в тактах TSC
double сlockIntervalUsingTSC() {
	__int64 t_start, t_finish;
	clock_t t_clock = clock();
	while (clock() < t_clock + 1);
	t_start = __rdtsc();
	while (clock() < t_clock + 2);
	t_finish = __rdtsc();
	return double(t_finish - t_start);
};

double sum = 0;
// Длительнность линейной программы из 1000 команд подсуммирования 1.0 к статической переменной
double delayesADD() {
	__int64 t_start;
	t_start = __rdtsc();
	Repeat1000(sum += 1.0;);
	return double(__rdtsc() - t_start);
};

double Integral1000() {
	__int64 t_start;
	t_start = __rdtsc();
	double start = 0;
	double end = 1;
	double counter = start;
	double value = 0;
	Repeat1000({
		value += sin(counter);
		counter += 0.001;
	});
	return double(__rdtsc() - t_start);
}

double Integral10000() {
	__int64 t_start;
	t_start = __rdtsc();
	double counter = 0;
	double value = 0;
	Repeat10000({
		value += sin(counter) * 0.0001;
		counter += 0.0001;
		});
	return double(__rdtsc() - t_start);
}

unsigned __int64 fibRecursive(unsigned __int64 n) {
	if (n < 2) return n;
	return fibRecursive(n - 1) + fibRecursive(n - 2);
}
//y = cos(x) - sin(x) 
double Integral(int n) {
	double value = 0;

	for (int i = 0; i <= n; i++) {
		value += (cos(1 / n * i) - sin(1 / n * i)) * 1 / n;
	}

	return value;
}

double integralQPC() {
	LARGE_INTEGER t_start, t_finish, freq;
	__int64 t_code;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&t_start);

	Integral(1000000);

	QueryPerformanceCounter(&t_finish);
	t_code = t_finish.QuadPart - t_start.QuadPart;
	return double(t_code) / freq.QuadPart;
}

double fibRecursive10QPC() {
	LARGE_INTEGER t_start, t_finish, freq;
	__int64 t_code;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&t_start);
	int* results = new int[100];
	for (int i = 0; i < 100; i++) {
		results[i] = fibRecursive(10);
	}
	

	QueryPerformanceCounter(&t_finish);
	t_code = t_finish.QuadPart - t_start.QuadPart;
	return double(t_code) / freq.QuadPart;
}

int main() {
	Log log;
	Log suplog;
	double scale; // масштаб выводимых значений
	int nPasses = 5;  // число проходов при выполнении серий измерений
	setlocale(LC_CTYPE, "rus");
	cpuInfo();

	cout << "\n\nОценка повторяемости 1000 измерений clock - интервала через QPC без фильтрации\n";
	scale = MCS_IN_SEC; // Значения будем выводить в микросекундах
	log.config({ {PREC_AVG, 2},  // Для среднего и СКО задаем более высокую точность
		{FILTR_MIN,0},{FILTR_MAX, 0} }); // нет фильтрации
	for (int n = 1; n <= nPasses; n++) {
		cout << "\nСерия " << n << endl << endl;
		log.series(true, 1000, сlockIntervalUsingQPC)
			.calc().stat(scale, "Число микросекунд в одном clock-интервале через QPC (без фильтрации)")
			.print(O_NATURAL, scale, 10).print(O_MIN, scale, 10).print(O_MAX, scale, 10);;
	}

	cout << "\n\nОценка повторяемости вычисления 10-го числа Фибоначчи через QPC без фильтрации\n";
	scale = NS_IN_SEC; // Значения будем выводить в микросекундах
	log.config({ {PREC_AVG, 2},  // Для среднего и СКО задаем более высокую точность
		{FILTR_MIN,0},{FILTR_MAX, 0} }); // нет фильтрации
	for (int n = 1; n <= nPasses; n++) {
		cout << "\nСерия " << n << endl << endl;
		log.series(true, 10, fibRecursive10QPC)
			.calc().stat(scale, "Число наносекунд затраченных на вычисление 10-го числа Фибоначчи через QPC (без фильтрации)")
			.print(O_NATURAL, scale, 10).print(O_MIN, scale, 10).print(O_MAX, scale, 10);;
	}

	cout << "\n\nОценка влияния параметров фильтрации\n";
	// Оценка влияния параметров фильтрации через отбрасывание наименьших и наибольших
	// в процессе измерения размера clock-интервала через QPC при размере серии 100
	scale = MCS_IN_SEC; // Значения будем выводить в микросекундах
	for (int n = 1; n <= nPasses; n++) {
		cout << "\nСерия " << n << endl << endl;
		log.series(true, 100, сlockIntervalUsingQPC).config({ {FILTR_MIN,n },{FILTR_MAX, n} })
			.calc().stat(MCS_IN_SEC, "Число микросекунд в одном clock-интервале через QPC")
			.print(O_NATURAL, MCS_IN_SEC, 10).print(O_MIN, MCS_IN_SEC, 10).print(O_MAX, MCS_IN_SEC, 10);;
	}

	// Оценка влияния размера серии измерений 
	// clock-интервала через число тактов TSC ,без фильтрации
	cout << "\n\nОценка повторяемости 1000 измерений clock - интервала через TSC без фильтрации\n";
	log.config({ {FILTR_MIN,0},{FILTR_MAX, 0} });
	scale = 1.;
	for (int n = 1; n <= nPasses; n++) {
		cout << "\nСерия " << n << endl << endl;
		log.series(true, 1000, сlockIntervalUsingTSC)
			.calc().stat(scale, "Число тактов TSC в одном clock-интервале")
			.print(O_NATURAL, scale, 10).print(O_MIN, scale, 10).print(O_MAX, scale, 10);;
	}
	__int64 freqTSC = log.avg * CLOCKS_PER_SEC;
	cout << endl << "freqTSC: " << freqTSC << endl;
	cout << "\n\nОценка повторяемости 100 измерений бесциклового подсуммирования 1000 констант\n";
	scale = NS_IN_SEC / freqTSC;
	for (int n = 1; n <= 5; n++) {
		cout << "\nСерия " << n << endl << endl;
		log.series(true, 100, delayesADD).config({ {FILTR_MIN, 2 * n},{FILTR_MAX, 2 * n} })
			.calc().stat(scale, "Задержки 1000-кратного исполнения sum += 1. в наносекундах")
			.print(O_NATURAL, scale, 10).print(O_MIN, scale, 10).print(O_MAX, scale, 10);
	}

	cout << "\n\nОценка повторяемости 100 измерений определенного интеграла синуса на отрезке от 0 до 1\n";
	scale = NS_IN_SEC / freqTSC;
	for (int n = 1; n <= 5; n++) {
		cout << "\nСерия " << n << endl << endl;
		log.series(true, 100, Integral1000).config({ {FILTR_MIN, 2 * n},{FILTR_MAX, 2 * n} })
			.calc().stat(scale, "Задержки 1000-кратного исполнения вычисления определенного интеграла функции sin(x) от 0 до 1 в наносекундах")
			.print(O_NATURAL, scale, 10).print(O_MIN, scale, 10).print(O_MAX, scale, 10);
	}

	return 0;
}