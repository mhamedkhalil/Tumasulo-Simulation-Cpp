#include<iostream>
#include<fstream>
#include<sstream>
#include<unordered_map>
#include<exception>
#include"ReservationStation.h"
using namespace std;

#define REGISTER_NUM 16

struct Reg
{
    ROBEntry* Qi = nullptr;
    int value = 0;
    bool busy = false;
};
vector<Reg> RF(REGISTER_NUM);
void clearRegister(Reg* target)
{
    target->busy = false;
    target->Qi = nullptr;
    target->value = 0;
}

bool speculate = 0;

int LOAD_UNITS, LOAD_UNITS_CYCLES; 
int STORE_UNITS, STORE_UNITS_CYCLES;
int BEQ_UNITS, BEQ_UNITS_CYCLES;
int CALL_UNITS, CALL_UNITS_CYCLES;
int ADD_UNITS, ADD_UNITS_CYCLES; 
int NAND_UNITS, NAND_UNITS_CYCLES;
int MUL_UNITS, MUL_UNITS_CYCLES;

int DM[128];
int Clock = 0;
int PC = 0;

vector<reservationStation> loadStation;
vector<reservationStation> storeStation;
vector<reservationStation> beqStation;
vector<reservationStation> callStation;
vector<reservationStation> addStation;
vector<reservationStation> nandStation;
vector<reservationStation> mulStation;

unordered_map<string,vector<reservationStation>*> Stations;
unordered_map<string, int> Stations_Time; 

struct Instruction
{
    string Op;
    int dest;
    int src1;
    int src2;
};
vector<Instruction> Instructions;

void Parser(string& instructionsFile)
{
    unordered_map<string,int> Labels;
    /*
    LW rd, imm(rs1)
    SW rs2, imm(rs1)
    BEQ rs1, rs2, imm
    JAL rs1, Label/imm
    ADD rd, rs1, rs2
    ADDI rs, rs1, imm
    NAND rd, rs1, rs2
    MUL rd, rs1, rs2
    */
    ifstream instStream(instructionsFile);
    if (!instStream.is_open())
    {
        cerr << "Error opening the instructions file " << instructionsFile << endl;
        return;
    }

    string line;
    int position = 0;
    //Parse for labels
    while(getline(instStream, line))
    {
        string temp;
        istringstream iss(line);
        if (!(iss >> temp))
            continue;

        if(temp != "LW" && temp != "SW" && temp != "BEQ" && temp != "JAL"
            && temp != "ADD" && temp != "ADDI" && temp != "NAND" && temp != "MUL")
        {
            Labels[temp] = position;
        }
        else
            ++position;
    }

    instStream.clear();
    instStream.seekg(0, ios::beg);  
    while (getline(instStream, line))
    {
        istringstream iss(line);
        Instruction inst;
        if (!(iss >> inst.Op))
            continue;

        if (inst.Op == "LW")
        {
            // LW rd, imm(rs1)
            string rd, address;
            if (iss >> rd >> address) // Parse rd and imm(rs1)
            {
                size_t leftParen = address.find('(');
                size_t rightParen = address.find(')');

                if (leftParen != string::npos && rightParen != string::npos && rightParen > leftParen)
                {
                    int imm = stoi(address.substr(0, leftParen)); // Extract immediate
                    string rs1 = address.substr(leftParen + 1, rightParen - leftParen - 1); // Extract rs1

                    inst.dest = stoi(rd.substr(1));   // Extract register number from "rX"
                    inst.src1 = stoi(rs1.substr(1));  // Extract register number from "rX"
                    inst.src2 = imm;                  // Store immediate in src2
                }
                else
                {
                    cerr << "Error parsing LW instruction: " << line << endl;
                    continue;
                }
            }
            else
            {
                cerr << "Error parsing LW instruction: " << line << endl;
                continue;
            }
        }
        else if (inst.Op == "SW")
        {
            // SW rs2, imm(rs1)
            string rs2, address;
            if (iss >> rs2 >> address) // Parse rs2 and imm(rs1)
            {
                size_t leftParen = address.find('(');
                size_t rightParen = address.find(')');

                if (leftParen != string::npos && rightParen != string::npos && rightParen > leftParen)
                {
                    int imm = stoi(address.substr(0, leftParen)); // Extract immediate
                    string rs1 = address.substr(leftParen + 1, rightParen - leftParen - 1); // Extract rs1

                    inst.src2 = stoi(rs2.substr(1));  // Extract register number from "rX"
                    inst.src1 = stoi(rs1.substr(1));  // Extract register number from "rX"
                    inst.dest = imm;                  // Use dest to store the immediate value
                }
                else
                {
                    cerr << "Error parsing SW instruction: " << line << endl;
                    continue;
                }
            }
            else
            {
                cerr << "Error parsing SW instruction: " << line << endl;
                continue;
            }
        }

        else if (inst.Op == "BEQ")
        {
            // BEQ rs1, rs2, imm
            int imm;
            string rs1, rs2;

            if (iss >> rs1 >> rs2 >> imm)
            {
                inst.src1 = stoi(rs1.substr(1)); 
                inst.src2 = stoi(rs2.substr(1)); 
                inst.dest = imm; // Use dest to store the branch target
            }
            else
            {
                cerr << "Error parsing BEQ instruction: " << line << endl;
                continue;
            }
        }
        else if (inst.Op == "JAL")
        {
            // JAL rs1, Label/imm
            string immOrLabel;
            string rs1;

            if (iss >> rs1 >> immOrLabel)
            {
                inst.src1 = stoi(rs1.substr(1));

                // Check if the target is a label or immediate value
                if (Labels.find(immOrLabel) != Labels.end())
                {
                    inst.dest = Labels[immOrLabel];
                }
                else
                {
                    inst.dest = stoi(immOrLabel);
                }
                inst.src2 = -1;
            }
            else
            {
                cerr << "Error parsing JAL instruction: " << line << endl;
                continue;
            }
        }
        else if (inst.Op == "ADD" || inst.Op == "NAND" || inst.Op == "MUL")
        {
            // ADD/NAND/MUL rd, rs1, rs2
            string rd, rs1, rs2;

            if (iss >> rd >> rs1 >> rs2)
            {
                inst.dest = stoi(rd.substr(1)); // Extract register number from "rX"
                inst.src1 = stoi(rs1.substr(1)); // Extract register number from "rX"
                inst.src2 = stoi(rs2.substr(1)); // Extract register number from "rX"
            }
            else
            {
                cerr << "Error parsing " << inst.Op << " instruction: " << line << endl;
                continue;
            }
        }
        else if (inst.Op == "ADDI")
        {
            // ADDI rs, rs1, imm
            int imm;
            string rd, rs1;

            if (iss >> rd >> rs1 >> imm)
            {
                inst.dest = stoi(rd.substr(1));  // Extract register number from "rX"
                inst.src1 = stoi(rs1.substr(1)); // Extract register number from "rX"
                inst.src2 = imm;                 // Use src2 to store the immediate value
            }
            else
            {
                cerr << "Error parsing ADDI instruction: " << line << endl;
                continue;
            }
        }
        else
            continue;

        // Add the successfully parsed instruction to the list
        Instructions.push_back(inst);
    }

    instStream.close();
}

void UserInput()
{
    //Assuming the user will input only integers as required
    cout << "Choose the Hardware components that you require for the simulation:\n";
    cout << "Number of Load Stations: "; cin >> LOAD_UNITS;
    cout << "Number of Store Stations: "; cin >> STORE_UNITS;
    cout << "Number of BEQ Stations: "; cin >> BEQ_UNITS;
    cout << "Number of Call/Return Stations: "; cin >> CALL_UNITS;
    cout << "Number of ADD/ADDI Stations: "; cin >> ADD_UNITS;
    cout << "Number of NAND Stations: "; cin >> NAND_UNITS;
    cout << "Number of Multiplication Stations: "; cin >> MUL_UNITS;
    cout << "Number of ROB entries:"; cin >> ROB_ENTRIES;
    cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    cout << "Initializing Hardware...\n";
    loadStation.resize(LOAD_UNITS);
    storeStation.resize(STORE_UNITS);
    beqStation.resize(BEQ_UNITS);
    callStation.resize(CALL_UNITS);
    addStation.resize(ADD_UNITS);
    nandStation.resize(NAND_UNITS);
    mulStation.resize(MUL_UNITS);
    ROB.resize(ROB_ENTRIES,ROBEntry());

    Stations["LW"] = &loadStation;
    Stations["SW"] = &storeStation;
    Stations["BEQ"] = &beqStation;
    Stations["JAL"] = &callStation;
    Stations["ADD"] = &addStation;
    Stations["ADDI"] = &addStation;
    Stations["NAND"] = &nandStation;
    Stations["MUL"] = &mulStation;

    cout << "Choose the execution time of each station (in clock cycles):\n";
    cout << "EXEC time of LOAD: "; cin >> LOAD_UNITS_CYCLES;
    cout << "EXEC time of STORE: "; cin >> STORE_UNITS_CYCLES;
    cout << "EXEC time of BRANCH: "; cin >> BEQ_UNITS_CYCLES;
    cout << "EXEC time of CALL: "; cin >> CALL_UNITS_CYCLES;
    cout << "EXEC time of ADD/ADDI: "; cin >> ADD_UNITS_CYCLES;
    cout << "EXEC time of NAND: "; cin >> NAND_UNITS_CYCLES;
    cout << "EXEC time of MUL: "; cin >> MUL_UNITS_CYCLES;
    cout << "Clock cycles registered successfully!\n";
    cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

    Stations_Time["LW"] = LOAD_UNITS_CYCLES;
    Stations_Time["SW"] = STORE_UNITS_CYCLES;
    Stations_Time["BEQ"] = BEQ_UNITS_CYCLES;
    Stations_Time["JAL"] = CALL_UNITS_CYCLES;
    Stations_Time["ADD"] = ADD_UNITS_CYCLES;
    Stations_Time["ADDI"] = ADD_UNITS_CYCLES;
    Stations_Time["NAND"] = NAND_UNITS_CYCLES;
    Stations_Time["MUL"] = MUL_UNITS_CYCLES;
}

void RollBack(int correctPC) //to be called on misprediction (when branching)
{
    while (ROB_head != ROB_tail) 
    {
        ROBEntry& entry = ROB[ROB_head];
        if (entry.speculative)
        {
            for(auto &i : RF)
            {
                if(i.Qi == &entry)
                {
                    clearRegister(&i);
                }
            }
            entry.clearROB();
            ROB_head = (ROB_head + 1) % ROB.size();
        }
        else
        {
            break; //stop clearing once we encounter non-speculative entries
        }
    }

    if(ROB_head == ROB_tail) //handle case where head reaches tail
    {
        ROBEntry& entry = ROB[ROB_head];
        if (entry.speculative)
        {
            entry.clearROB();
            ROB_head = (ROB_head + 1) % ROB.size();
        }
    }

    for(auto &stationGroup : Stations) //clear all reservation stations not needed anymore
    {
        for(auto &station : *stationGroup.second)
        {
            if(station.speculative)
                station.flush();
        }
    }

    PC = correctPC;
    speculate = 0;
}

int ISSUE()
{
    if(PC/4 >= Instructions.size())
        return 0; //No more instructions to isssue
        
    Instruction currentInst = Instructions[PC/4];
    vector<reservationStation>& stationGroup = *Stations[currentInst.Op];
    for(auto &station: stationGroup)
    {
        if(!station.Busy)
        {
            
            if ((ROB_tail + 1) % ROB_ENTRIES == ROB_head && !ROB[ROB_tail].free) 
            {
                cout << "ROB is full. Stalling.\n";
                return -1; // Stall
            }

            ROBEntry& robEntry = ROB[ROB_tail];
            ROB_tail = (ROB_tail + 1) % ROB_ENTRIES == ROB_head ? ROB_tail : (ROB_tail + 1) % ROB_ENTRIES;
            robEntry = {PC / 4, currentInst.Op, currentInst.dest, 0, false, false, speculate, &station};

            RF[currentInst.dest].Qi = &robEntry;
            RF[currentInst.dest].busy = true;

            station.Op = currentInst.Op;
            station.Busy = true;
            station.ready = false;
            station.Qj = RF[currentInst.src1].Qi;
            station.Qk = RF[currentInst.src2].Qi;
            station.Vj = (station.Qj == nullptr) ? RF[currentInst.src1].value : 0;
            station.Vk = (station.Qk == nullptr) ? RF[currentInst.src2].value : 0;
            if(station.Op == "LW")
                station.A = currentInst.src2;
            else 
                station.A = currentInst.dest;
            station.speculative = speculate;
            station.remainingTime = Stations_Time[station.Op];
            if(currentInst.Op == "BEQ")
                speculate = 1;
            cout << "Issued instruction: " << currentInst.Op << " to a reservation station.\n";
            PC += 4; 
            return 1;
        }
    }
    cout << "No available reservation station for instruction: " << currentInst.Op << " --> Stalling" << endl;
    return -1;   
}

void EXECUTE() {
    for (auto& stationGroup : Stations) {
        for (auto& station : *stationGroup.second) {
          
            if (station.execute()) {
              
                if (station.Op == "ADD" || station.Op == "ADDI") {
                    if(station.remainingTime == 0)
                    {
                        station.Vj = station.Vj + station.Vk;
                        station.ready = 1;
                        cout << "Executed ADD/ADDI: " << station.Vj << endl;
                        cout << "Clock: " << Clock << endl;
                    }
                    else 
                        station.remainingTime--;
                }
                else if (station.Op == "NAND") {
                    if(station.remainingTime == 0) {
                    station.Vj = ~(station.Vj & station.Vk); 
                    station.ready = 1;
                    cout << "Executed NAND: " << station.Vj << endl;
                    cout << "Clock: " << Clock << endl;
                    }
                    else 
                        station.remainingTime--;
                }
                else if (station.Op == "MUL") {
                    if(station.remainingTime == 0) {
                    station.Vj = station.Vj * station.Vk;
                    station.ready = 1;
                    cout << "Executed MUL: " << station.Vj << endl;
                    cout << "Clock: " << Clock << endl;
                    }
                    else 
                        station.remainingTime--;
                }
                else if (station.Op == "LW") {
                    if(station.remainingTime == LOAD_UNITS_CYCLES - ADD_UNITS_CYCLES)
                    {
                        station.A = station.A + station.Vj;
                        cout << "Computed Address of " << station.Op << " " << station.A << endl;
                        cout << "Clock: " << Clock << endl;
                    }
                    if(station.remainingTime == 0) {
                    cout << "DM content: " << DM[station.A] << endl;
                    station.Vj = DM[station.A];
                    station.ready = 1;
                    cout << "Executed LW: " << station.Vj << endl;
                    cout << "Clock: " << Clock << endl;
                    }
                    else {
                        station.remainingTime--;
                        }
                }
                else if (station.Op == "SW") {
                   if(station.remainingTime == STORE_UNITS_CYCLES - ADD_UNITS_CYCLES)
                    {
                        station.A = station.A + station.Vj;
                        cout << "Computed Address of " << station.Op << endl;
                        cout << "Clock: " << Clock << endl;
                    }
                    if(station.remainingTime == 0) {
                    station.Vj = station.Vk;
                    station.ready = 1; 
                    cout << "Executed SW: " << station.Vj << endl;
                    cout << "Clock: " << Clock << endl;
                    }
                    station.remainingTime--;
                }
                else if (station.Op == "BEQ") {
                    if(station.remainingTime == 0) {
                        if (station.Vj == station.Vk) {
                            RollBack(station.A);
                            station.ready = 1; 
                            cout << "Executed BEQ: Branching to " << PC << endl;
                            cout << "Rolling Back!" << endl;
                            cout << "Clock: " << Clock << endl;
                        }
                        else
                        {
                            cout << "BEQ predicted right! Proceeding normally" << endl;
                            cout << "Clock: " << Clock << endl;
                            speculate = 0;
                            for(auto &entry : ROB)
                            {
                                if(entry.speculative)
                                    entry.speculative = 0;
                            }
                        }
                    }
                    else
                        station.remainingTime--;
                }
                else if (station.Op == "JAL") {
                    if(station.remainingTime == 0) {
                    PC = station.A*4;
                    station.Vj = PC + 4;
                    station.ready = 1; 
                    cout << "Executed JAL: Jumping to " << PC << endl;
                    }
                    else
                        station.remainingTime--;
                } 
            }
        }
    }
    cout << "No instructions ready for execution.\n";
}

void WB() {

    for(auto &rob : ROB)
    {
        // cout << "rob" << " result: " << rob.station->write() << endl;
        if(rob.station->write())
        {
            rob.value = rob.station->Vj;
            rob.ready = 1;
            cout << "Wrote result of " << rob.station->Op << " (" << rob.station->Vj << ")" << endl;
            rob.station->flush();
        }
    }

    for(auto &stationGroup : Stations)
    {
        for(auto &station : *stationGroup.second)
        {
            // cout << "HEREEEE" << endl;
            if(station.Busy)
            {
                try{
                if(station.Qj != nullptr  && station.Qj->ready)
                {
                    station.Vj = station.Qj->value;
                    cout << "Assigned value of " << station.Qj->Op << " to pending " << station.Op << endl;
                    station.Qj = nullptr;
                }
                if(station.Qk != nullptr && station.Qk->ready)
                {
                    station.Vk = station.Qk->value;
                    cout << "Assigned value of " << station.Qk->Op << " to pending " << station.Op << endl;
                    station.Qk = nullptr;
                }
                }
                catch(exception&)
                {
                    continue;
                }
            }
        }
    }
    cout << "No instructions ready for write-back.\n"; 
}

void COMMIT(bool *allCommitted) {

    if (ROB[ROB_head].free || !ROB[ROB_head].ready) {
        cout << "No instruction ready to commit." << endl;
        return;
    }

    ROBEntry& entry = ROB[ROB_head];

    // Commit action depends on the operation type
    if (entry.Op == "SW") {
        // For store, write the value to memory
        DM[entry.dest] = entry.value;
        cout << "Committed SW: Address " << entry.dest << " updated with value " << entry.value << endl;
    } else {
        // For other operations, update the register file
        if (entry.dest < REGISTER_NUM && entry.dest >= 0) {
            RF[entry.dest].value = entry.value;
            RF[entry.dest].Qi = nullptr;
            RF[entry.dest].busy = false;
            cout << "Committed " << entry.Op << ": Register r" << entry.dest << " updated with value " << entry.value << endl;
        }
    }

    // Clear the ROB entry
    entry.clearROB();
    ROB_head = (ROB_head + 1) % ROB.size();

    *allCommitted = true;
    for (auto &i : ROB) {
        if (!i.free) {
            *allCommitted = false;
            break;
        }
    }
}


int main() {

    UserInput();

    RF[0].value = 0;
    RF[0].busy = 0;
    RF[0].Qi = nullptr;
    DM[0] = 1;
    DM[1] = 2;
    DM[11] = 3;
    // Create a sample instruction file
    string fileName = "instructions.txt";
    // Parse the file
    Parser(fileName);

    // Print the parsed instructions
    cout << "Parsed Instructions:\n";
    for (size_t i = 0; i < Instructions.size(); i++) {
        const Instruction& inst = Instructions[i];
        cout << i << ": " << inst.Op << " "
             << "dest=" << inst.dest << ", "
             << "src1=" << inst.src1 << ", "
             << "src2=" << inst.src2 << endl;
    }
    bool allIssued = 0, allCommitted = 0;
    cout << "=== Starting Program ===\n";
    while (true) {
        int result = ISSUE();

        if (result == 0) {
            cout << "All instructions issued successfully.\n";
            allIssued = 1;
        } else if (result == -1) {
            cout << "Stall detected. Unable to issue instruction.\n";
        } else if (result == 1) {
            cout << "Instruction issued.\n";
        }

        EXECUTE();
        WB();
        cout << "Before calling Commit" << endl;
        COMMIT(&allCommitted);
        cout << "After calling Commit" << endl;
        if(allIssued && allCommitted)
            break;
        // Simulate clock tick
        Clock++;
        if (Clock > 30) {
            cout << "Exceeded clock limit. Stopping simulation.\n";
            break;
        }
    }
    cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    cout << "\nFinal PC: " << PC << endl;
    cout << "Total Clock Cycles: " << Clock << endl;
    cout << "Final Registers content:\n";
    for(int i=0; i<16; i++)
    {
        cout << i << ": " << RF[i].value << endl;
    }
    cout << "DM[17]: " << DM[17] << endl;
    return 0;
}
  