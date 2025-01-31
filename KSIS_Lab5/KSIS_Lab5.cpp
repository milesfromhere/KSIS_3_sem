//изучение методов диагностики TCP/IP сетей
// Подключение необходимых заголовочных файлов
#include <iostream>
using namespace std;
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>

// Связывание библиотек с помощью pragma-директив
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma warning (disable: 4996) // Отключение предупреждений о устаревших функциях
#define IP_STATUS_BASE 11000 
#define IP_SUCCESS 0
#define IP_DEST_NET_UNREACHABLE 11002
#define IP_DEST_HOST_UNREACHABLE 11003 
#define IP_DEST_PROT_UNREACHABLE 11004
#define IP_DEST_PORT_UNREACHABLE 11005
#define IP_REQ_TIMED_OUT 11010 
#define IP_BAD_REQ 11011
#define IP_BAD_ROUTE 11012
#define IP_TTL_EXPIRED_TRANSIT 11013

// Структура для параметров ICMP-запроса
IP_OPTION_INFORMATION option = { 255, 255, 240, 0, 0 };

// Функция для выполнения операции пинга
//cHost (const char*): Адрес хоста, на который будет выполнен пинг (IP-адрес или доменное имя).
//Timeout (unsigned int): Время ожидания ответа (в миллисекундах) после отправки запроса.
//RequestCount (unsigned int): Количество посылаемых запросов.
void Ping(const char* cHost, unsigned int Timeout, unsigned int RequestCount)
{
    // Создание дескриптора ICMP-сервиса
    HANDLE hIP = IcmpCreateFile();

    // Проверка на успешное создание дескриптора ICMP-сервиса
    if (hIP == INVALID_HANDLE_VALUE)
    {
        WSACleanup(); // Очистка сокета
        return;
    }

    char SendData[32] = "Data for ping"; // Буфер для передачи данных
    int LostPacketsCount = 0; // Счетчик потерянных пакетов
    unsigned int MaxMS = 0; // Максимальное время ответа (мс)
    int MinMS = -1; // Минимальное время ответа (мс)

    // Выделение памяти для буфера ответа
    //Эта строка выделяет память для структуры ICMP_ECHO_REPLY и буфера SendData, сохраняя указатель на эту память в переменной pIpe, чтобы использовать его для хранения ответов на ICMP-запросы.
    PICMP_ECHO_REPLY pIpe = (PICMP_ECHO_REPLY)GlobalAlloc(GHND, sizeof(ICMP_ECHO_REPLY) + sizeof(SendData));

    // Проверка на успешное выделение памяти
    if (pIpe == 0)
    {
        IcmpCloseHandle(hIP); // Закрытие дескриптора ICMP-сервиса
        WSACleanup(); // Очистка сокета
        return;
    }

    // Настройка буфера ответа для передачи данных и размера буфера
    pIpe->Data = SendData;//станавливают буфер данных SendData в структуре pIpe
    pIpe->DataSize = sizeof(SendData);//определяют размер этого буфера в DataSize
    unsigned long ipaddr = inet_addr(cHost); // Преобразование адреса хоста в числовой формат

    for (unsigned int c = 0; c < RequestCount; c++)
    {
        // Отправка ICMP-запроса на хост
        int dwStatus = IcmpSendEcho(
            hIP,           // Дескриптор ICMP-сервиса
            ipaddr,        // Адрес хоста
            SendData,      // Данные для отправки
            sizeof(SendData), // Размер отправляемых данных
            &option,       // Параметры ICMP-запроса
            pIpe,          // Буфер для получения ответа
            sizeof(ICMP_ECHO_REPLY) + sizeof(SendData), // Размер буфера ответа
            Timeout        // Время ожидания ответа
        );
        if (dwStatus > 0)
        {
            // Обработка полученных ответов
            for (int i = 0; i < dwStatus; i++) {
                // Вывод информации о полученных ответах
                unsigned char* pIpPtr = (unsigned char*)&pIpe->Address;
                cout << "Ответ от " << (int)*(pIpPtr)
                    << "." << (int)*(pIpPtr + 1)
                    << "." << (int)*(pIpPtr + 2)
                    << "." << (int)*(pIpPtr + 3)
                    << ": число байт = " << pIpe->DataSize
                    << " время = " << pIpe->RoundTripTime
                    << "мс TTL = " << (int)pIpe->Options.Ttl;
                // Обновляем максимальное время ответа, если новое время больше текущего максимального
                MaxMS = (MaxMS > pIpe->RoundTripTime) ? MaxMS : pIpe->RoundTripTime;
                // Обновляем минимальное время ответа, если новое время меньше текущего минимального, но только если MinMS >= 0
                MinMS = (MinMS < (int)pIpe->RoundTripTime && MinMS >= 0) ? MinMS : pIpe->RoundTripTime;
                cout << endl;
            }
        }
        else
        {
            // Обработка потерянных пакетов и ошибок
            if (pIpe->Status)
            {
                // Если статус ответа не равен нулю, это указывает на ошибку или потерю пакета
                LostPacketsCount++; // Увеличиваем счетчик потерянных пакетов

                // Оцениваем статус и выводим сообщение в зависимости от его значения
                switch (pIpe->Status)
                {
                case IP_DEST_NET_UNREACHABLE:
                case IP_DEST_HOST_UNREACHABLE:
                case IP_DEST_PROT_UNREACHABLE:
                case IP_DEST_PORT_UNREACHABLE:
                    cout << "Remote host may be down." << endl; // Хост удален и, возможно, недоступен
                    break;
                case IP_REQ_TIMED_OUT:
                    cout << "Request timed out." << endl; // Запрос не получил ответа в установленный таймаут
                    break;
                case IP_TTL_EXPIRED_TRANSIT:
                    cout << "TTL expired in transit." << endl; // Время жизни (TTL) истекло в процессе передачи
                    break;
                default:
                    cout << "Error code: " << pIpe->Status << endl; // Выводим стандартное сообщение об ошибке
                    break;
                }

            }
        }
    }

    // Закрытие дескриптора ICMP-сервиса и очистка сокса
    IcmpCloseHandle(hIP);
    WSACleanup();

    // Подсчет статистики и вывод результатов пинга
    if (MinMS < 0) MinMS = 0;//если значение MinMS (минимальное время ответа) меньше нуля. Если так, то устанавливает MinMS в ноль, чтобы избежать отрицательного значения.
    unsigned char* pByte = (unsigned char*)&pIpe->Address;//Создается указатель pByte, который указывает на начало байтового представления IP-адреса, полученного в ответе на пинг.
    cout << "Статистика Ping "
        << (int)*(pByte)
        << "." << (int)*(pByte + 1)
        << "." << (int)*(pByte + 2)
        << "." << (int)*(pByte + 3) << endl;
    cout << "\tПакетов: отправлено = " << RequestCount
        << ", получено = " << RequestCount - LostPacketsCount
        << ", потеряно = " << LostPacketsCount
        << "<" << (int)(100 / (float)RequestCount) * LostPacketsCount
        << " % потерь>, " << endl;
    cout << "Приблизительное время приема-передачи:"
        << endl << "Минимальное = " << MinMS
        << "мс, Максимальное = " << MaxMS
        << "мс, Среднее = " << (MaxMS + MinMS) / 2
        << "мс" << endl;
}


int main(int argc, char** argv)
{
    setlocale(LC_ALL, "RUS");

    // Получение аргументов из командной строки
    const char* par0 = argv[1];
    int par1 = atoi(argv[2]);
    int par2 = atoi(argv[3]);

    // Вызов функции Ping с передачей аргументов из командной строки
    Ping(par0, par1, par2);
    // Пример вызова функции с жестко заданными аргументами
    // Ping("81.19.70.1", 60, 10);

    system("pause");
    return 0;
}