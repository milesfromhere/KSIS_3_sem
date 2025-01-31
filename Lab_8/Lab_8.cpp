#include <iostream>
#include <vector>
#include <sstream>
#include <bitset>
#include <Windows.h>

using namespace std;

// Функция для разделения строки по символу-разделителю
vector<string> split(const string& str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool isValidIP(const string& ip) {
    vector<string> octets = split(ip, '.');
    if (octets.size() != 4) return false;

    for (const string& octet : octets) {
        if (octet.empty()) return false;

        int value;
        try {
            value = stoi(octet);
        }
        catch (...) {
            return false;
        }

        if (value < 0 || value > 255) return false;
    }
    return true;
}

// Проверка правильности маски
bool isValidMask(const string& mask) {
    if (!isValidIP(mask)) return false;

    vector<string> octets = split(mask, '.');
    unsigned int maskBits = 0;

    // Преобразуем маску в битовое представление
    for (const string& octet : octets) {
        maskBits = (maskBits << 8) | stoi(octet);
    }

    // Проверяем непрерывность единиц
    bool zeroFound = false;
    for (int i = 31; i >= 0; --i) {
        if (maskBits & (1 << i)) {
            if (zeroFound) return false; // После нуля не должно быть единиц
        }
        else {
            zeroFound = true;
        }
    }
    return true;
}

string calculateNetworkID(const string& ip, const string& mask) {
    vector<string> ipOctets = split(ip, '.');
    vector<string> maskOctets = split(mask, '.');

    vector<int> networkID(4);
    for (int i = 0; i < 4; ++i) {
        networkID[i] = stoi(ipOctets[i]) & stoi(maskOctets[i]);
    }

    return to_string(networkID[0]) + "." + to_string(networkID[1]) + "." +
        to_string(networkID[2]) + "." + to_string(networkID[3]);
}

string calculateBroadcastAddress(const string& ip, const string& mask) {
    vector<string> ipOctets = split(ip, '.');
    vector<string> maskOctets = split(mask, '.');

    vector<int> broadcast(4);
    for (int i = 0; i < 4; ++i) {
        broadcast[i] = stoi(ipOctets[i]) | (~stoi(maskOctets[i]) & 0xFF);
    }

    return to_string(broadcast[0]) + "." + to_string(broadcast[1]) + "." +
        to_string(broadcast[2]) + "." + to_string(broadcast[3]);
}

string calculateHostID(const string& ip, const string& mask) {
    vector<string> ipOctets = split(ip, '.');
    vector<string> maskOctets = split(mask, '.');

    vector<int> hostID(4);
    for (int i = 0; i < 4; ++i) {
        hostID[i] = stoi(ipOctets[i]) & (~stoi(maskOctets[i]) & 0xFF);
    }

    return to_string(hostID[0]) + "." + to_string(hostID[1]) + "." +
        to_string(hostID[2]) + "." + to_string(hostID[3]);
}

int main() {
    setlocale(LC_ALL, "rus");
    string ip, mask;

    cout << "Введите IP-адрес: ";
    cin >> ip;

    cout << "Введите маску: ";
    cin >> mask;

    if (!isValidIP(ip)) {
        cout << "Ошибка: некорректный IP-адрес." << endl;
        return 0;
    }

    if (!isValidMask(mask)) {
        cout << "Ошибка: некорректная маска." << endl;
        return 0;
    }

    string networkID = calculateNetworkID(ip, mask);
    string broadcastAddress = calculateBroadcastAddress(ip, mask);
    string hostID = calculateHostID(ip, mask);

    cout << "Network ID: " << networkID << endl;
    cout << "Широковещательный адрес: " << broadcastAddress << endl;
    cout << "Host ID: " << hostID << endl;

    return 0;
}
