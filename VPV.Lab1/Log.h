#pragma once
#include <iostream> 
#include <vector> 
#include <sstream> 
#include <numeric> 
#include <cmath> 
#include <intrin.h>
#include <algorithm>
#include <thread>
#include <omp.h>
#include <iomanip>
#include <map>


using namespace std;

// Код упорядочевания: естественный, вначале минимальные, вначале максимальные
enum { O_NATURAL, O_MIN, O_MAX };
/* Параметры конфигурирования формата распечатки и фильтрации
   PREC_VAL - число знаков после запятой в распечатки сохраняемых в протоколе значений
   PREC_AVG - число знаков после запятой в распечатке
   FILTR_MIN, FILTR_MAX - число отбрасываемых наименьших и наибольших перед подсчетом среднего и СКО
*/
enum Config { CONFIG_FIRST = 0, PREC_VAL = 0, PREC_AVG, FILTR_MIN, FILTR_MAX, CONFIG_SIZE };

// Получение информации о процессоре через команду функцию __cpuid(unsigned *) info, funcCode),
// которая выполняет машинную  команду CPUID, предварительно загружая в EAX код функции funcCode. 
// CPUID возвращает 4 четырехбайтных кода: 0:EAX, 1:EBX, 2:ECX, 3:EDX, 
// а функция __cpuid перемещает эти слова по адресу info
void cpuInfo() {
    // int regs[4]; // 0:EAX, 1:EBX, 2:ECX, 3:EDX 
    char nameCPU[80]; // имя процессора
    // Получение имени процессора
    // Коды 0x80000002..0x80000004 позволяют получить полное имя CPU по 16 байтов
    __cpuid((int*)nameCPU, 0x80000002);
    __cpuid((int*)nameCPU + 4, 0x80000003);
    __cpuid((int*)nameCPU + 8, 0x80000004);
    int cores = thread::hardware_concurrency();

    cout << endl << nameCPU << "\tПотоков: " << cores << endl << endl;
}
// Класс протоколирования, обработки, вывода, организации серий измерений
class Log {
public:
    double dmin = 1.0E10, dmax = 0.; // минимальный и максимальный элемент
    double avg = 0., sqdev = 0., sqdev_perc = 0; // среднее, СКО, СКО%
    vector <double> arr;
    int conf[CONFIG_SIZE] = { 0, 0, 0, 0 };
    ostringstream msgfiltr;
    Log() {}

    Log& calc() {
        vector<double> vect = arr;
        msgfiltr.str("");
        if (conf[FILTR_MIN] > 0) {
            sort(vect.begin(), vect.end());
            vect.erase(vect.begin(), vect.begin() + conf[FILTR_MIN]);
            msgfiltr << "\nУдалено " << conf[FILTR_MIN] << " наименьших";
        }
        if (conf[FILTR_MAX] > 0) {
            sort(vect.begin(), vect.end(), [](double a, double b) {
                return a > b; });
            vect.erase(vect.begin(), vect.begin() + conf[FILTR_MAX]);
            msgfiltr << "  Удалено " << conf[FILTR_MAX] << " наибольших";
        }
        avg = accumulate(vect.begin(), vect.end(), 0.0, [&](double x, double y) {return x + y / vect.size(); });
        sqdev = sqrt(accumulate(vect.begin(), vect.end(), 0.,
            [&](double sq, double v) {double q = avg - v; return sq + q * q; })
            / vect.size());
        sqdev_perc = 100 * sqdev / avg;
        dmin = *min_element(vect.begin(), vect.end());
        dmax = *max_element(vect.begin(), vect.end());
        return *this;
    }

    Log& config(map< int, int > param) {
        for (auto it = param.begin(); it != param.end(); it++) {
            int n = (*it).first;
            if (n >= CONFIG_FIRST && n < CONFIG_SIZE)
                conf[n] = (*it).second;
        }
        return *this;
    }

    // Серия измерений, каждое из которых выполняется функцией meter, 
    // возвращающей время в секундах  
    Log& series(bool clear, int count, double (meter)()) {
        if (clear) arr.clear();
        for (int n = 0; n < count; n++) {
            arr.push_back(meter());
        }
        return *this;
    }

    // Добавление группы результатов измерения
    Log& set(vector <double> source) {
        for (double v : source)
            arr.push_back(v);
        return *this;
    }

    // Описательная статистика результатов измерений
    Log& stat(double scale, string unit) {
        cout.setf(ios::fixed);
        cout << "Статистика " << arr.size() << " измерений(" << unit << ")" << endl << msgfiltr.str() << endl
            << setprecision(conf[PREC_VAL]) << "Минимум: " << scale * dmin << "  Максимум: " << scale * dmax
            << setprecision(conf[PREC_AVG]) << "  Среднее: " << scale * avg
            << "  СКО: " << scale * sqdev << "  СКО%: " << sqdev_perc << endl;
        return *this;
    }

    // целочисленный вывод с масштабом scale первых len в одном из трех порадков:
    // O_NORM - естественный, O_MIN - минимальные, O_MAX - максимальные
    Log& print(int ord, double scale, unsigned len) {
        vector <double> vect = arr;
        string head, suffix = "";
        if (conf[FILTR_MIN] + conf[FILTR_MAX] > 0)
            suffix = " до фильтрации";
        int nsetw = int(ceil(log10(dmax * scale))) + conf[PREC_VAL] + 2;
        switch (ord) {
        case O_NATURAL:
            head = "Первые значения" + suffix + ": ";
            break;
        case O_MIN:
            sort(vect.begin(), vect.end());
            head = "Наименьшие" + suffix + ": ";
            break;
        case O_MAX:
            sort(vect.begin(), vect.end(), [](double a, double b) { return a > b; });
            head = "Наибольшие" + suffix + ": ";
            break;
        }
        cout.setf(ios::fixed);
        cout << head << setprecision(conf[PREC_VAL]) << endl;
        for (unsigned n = 0; n < min(len, unsigned(vect.size())); n++)
            cout << setw(nsetw) << scale * vect[n];
        cout << endl;
        return *this;
    }
};
