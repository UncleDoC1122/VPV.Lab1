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

// ��� ��������������: ������������, ������� �����������, ������� ������������
enum { O_NATURAL, O_MIN, O_MAX };
/* ��������� ���������������� ������� ���������� � ����������
   PREC_VAL - ����� ������ ����� ������� � ���������� ����������� � ��������� ��������
   PREC_AVG - ����� ������ ����� ������� � ����������
   FILTR_MIN, FILTR_MAX - ����� ������������� ���������� � ���������� ����� ��������� �������� � ���
*/
enum Config { CONFIG_FIRST = 0, PREC_VAL = 0, PREC_AVG, FILTR_MIN, FILTR_MAX, CONFIG_SIZE };

// ��������� ���������� � ���������� ����� ������� ������� __cpuid(unsigned *) info, funcCode),
// ������� ��������� ��������  ������� CPUID, �������������� �������� � EAX ��� ������� funcCode. 
// CPUID ���������� 4 �������������� ����: 0:EAX, 1:EBX, 2:ECX, 3:EDX, 
// � ������� __cpuid ���������� ��� ����� �� ������ info
void cpuInfo() {
    // int regs[4]; // 0:EAX, 1:EBX, 2:ECX, 3:EDX 
    char nameCPU[80]; // ��� ����������
    // ��������� ����� ����������
    // ���� 0x80000002..0x80000004 ��������� �������� ������ ��� CPU �� 16 ������
    __cpuid((int*)nameCPU, 0x80000002);
    __cpuid((int*)nameCPU + 4, 0x80000003);
    __cpuid((int*)nameCPU + 8, 0x80000004);
    int cores = thread::hardware_concurrency();

    cout << endl << nameCPU << "\t�������: " << cores << endl << endl;
}
// ����� ����������������, ���������, ������, ����������� ����� ���������
class Log {
public:
    double dmin = 1.0E10, dmax = 0.; // ����������� � ������������ �������
    double avg = 0., sqdev = 0., sqdev_perc = 0; // �������, ���, ���%
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
            msgfiltr << "\n������� " << conf[FILTR_MIN] << " ����������";
        }
        if (conf[FILTR_MAX] > 0) {
            sort(vect.begin(), vect.end(), [](double a, double b) {
                return a > b; });
            vect.erase(vect.begin(), vect.begin() + conf[FILTR_MAX]);
            msgfiltr << "  ������� " << conf[FILTR_MAX] << " ����������";
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

    // ����� ���������, ������ �� ������� ����������� �������� meter, 
    // ������������ ����� � ��������  
    Log& series(bool clear, int count, double (meter)()) {
        if (clear) arr.clear();
        for (int n = 0; n < count; n++) {
            arr.push_back(meter());
        }
        return *this;
    }

    // ���������� ������ ����������� ���������
    Log& set(vector <double> source) {
        for (double v : source)
            arr.push_back(v);
        return *this;
    }

    // ������������ ���������� ����������� ���������
    Log& stat(double scale, string unit) {
        cout.setf(ios::fixed);
        cout << "���������� " << arr.size() << " ���������(" << unit << ")" << endl << msgfiltr.str() << endl
            << setprecision(conf[PREC_VAL]) << "�������: " << scale * dmin << "  ��������: " << scale * dmax
            << setprecision(conf[PREC_AVG]) << "  �������: " << scale * avg
            << "  ���: " << scale * sqdev << "  ���%: " << sqdev_perc << endl;
        return *this;
    }

    // ������������� ����� � ��������� scale ������ len � ����� �� ���� ��������:
    // O_NORM - ������������, O_MIN - �����������, O_MAX - ������������
    Log& print(int ord, double scale, unsigned len) {
        vector <double> vect = arr;
        string head, suffix = "";
        if (conf[FILTR_MIN] + conf[FILTR_MAX] > 0)
            suffix = " �� ����������";
        int nsetw = int(ceil(log10(dmax * scale))) + conf[PREC_VAL] + 2;
        switch (ord) {
        case O_NATURAL:
            head = "������ ��������" + suffix + ": ";
            break;
        case O_MIN:
            sort(vect.begin(), vect.end());
            head = "����������" + suffix + ": ";
            break;
        case O_MAX:
            sort(vect.begin(), vect.end(), [](double a, double b) { return a > b; });
            head = "����������" + suffix + ": ";
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
