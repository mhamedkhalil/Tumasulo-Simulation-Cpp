#include<iostream>
#include<vector>
using namespace std;

int ROB_ENTRIES;

class reservationStation;
class ROBEntry
{
    public:
    int id;                  
    string Op;              
    int dest;                 
    int value;                
    bool ready;               
    bool free = 1;
    bool speculative = 0; 
    reservationStation* station;

    ROBEntry()
    {
        id = -1;
        Op = "";
        dest = 0;
        value = 0;
        ready = 0;
        free = 1;
        speculative = 0;
        station = nullptr;
    }

    ROBEntry(int ID, string OP, int DEST, int VALUE, bool READY, bool FREE, bool SPECULATIVE, reservationStation* STATION)
    {
        id = ID;
        Op = OP;
        dest = DEST;
        value = VALUE;
        ready = READY;
        free = FREE;
        station = STATION;
        speculative = SPECULATIVE;
    }

    void clearROB()
    {
        this->Op = "";
        this->free = 1;
        this->ready = 0;
        this->speculative = 0;
        this->value = 0;
        this->station = nullptr;
    }
};
vector<ROBEntry> ROB;
int ROB_head = 0, ROB_tail = 0;

class reservationStation
{
    public:
    string Op;
    int Vj, Vk;
    ROBEntry* Qj;
    ROBEntry* Qk;
    int A;
    bool Busy;
    bool speculative;
    int remainingTime;
    bool ready;

    reservationStation()
    {
        Op = "";
        Vj = -1;
        Vk = -1;
        Qj = nullptr;
        Qk = nullptr;
        A  = -1;
        Busy = 0;
        speculative = 0;
        remainingTime = 0;
        ready = 0;
    }

    bool execute()
    {
        if(this->Busy && this->Qj == nullptr && this->Qk == nullptr)
            return 1;
        return 0;
    }

    bool write()
    {
        if(this != nullptr && this->Busy && this->ready)
            return 1;
        return 0;
    }

    void flush()
    {
        this->Op = "";
        this->Vj = -1;
        this->Vk = -1;
        this->Qj = nullptr;
        this->Qk = nullptr;
        this->A  = -1;
        this->Busy = 0;
        this->speculative = 0;
        this->remainingTime = 0;
        this->ready = 0;
    }
};