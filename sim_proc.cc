#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include "sim_proc.h"
using namespace std;

void Fetch(std::fstream &fin){
	std::string line;
	Inst command;
	int counter = 0;

	if (fin.eof() || !DE.Isempty)
		return;

	while (getline(fin, line))
	{
		command.pc = PC;
		command.type = line[7] - '0';

		int destPos = 9;
		command.dst = command.dst_org = (line[destPos] == '-') ? -1 : atoi(line.substr(destPos, (line[destPos + 1] == ' ') ? 1 : 2).c_str());
		int rs1Pos = destPos + ((line[destPos + 1] == ' ') ? 2 : 3);
		command.rs1 = command.rs1_og = (line[rs1Pos] == '-') ? -1 : atoi(line.substr(rs1Pos, (line[rs1Pos + 1] == ' ') ? 1 : 2).c_str());
		int rs2Pos = rs1Pos + ((line[rs1Pos + 1] == ' ') ? 2 : 3);
		command.rs2 = command.rs2_og = (line[rs2Pos] == '-') ? -1 : atoi(line.substr(rs2Pos, 2).c_str());

		command.rs1_rdy = command.rs2_rdy = command.rs1_ROB = command.rs2_ROB = false;
		command.FE_start = Cycle;
		command.FE_cycles = 1;
		command.DE_start = Cycle + 1;

		switch (command.type)
		{
		case 0:
			command.count = 1;
			break;
		case 1:
			command.count = 2;
			break;
		case 2:
			command.count = 5;
			break;
		}
		PC++;
		DE.reg.push_back(command);
		counter++;

		if (DE.reg.size() == static_cast<unsigned>(Width))
			break;
	}

	if (counter != 0)
		DE.Isempty = false;
}

void Decode() {
    if (DE.Isempty || !RN.Isempty) {
        return;
    }

    for (std::vector<Instruction>::iterator it = DE.reg.begin(); it != DE.reg.end(); ++it) {
        it->RN_start = Cycle + 1;
        it->DE_cycles = it->RN_start - it->DE_start;
    }

	DE.Isempty = true;
    RN.Isempty = false;
    RN.reg.swap(DE.reg);
}

void Rename()
{
	if (!RN.Isempty)
	{
		if (RR.Isempty)
		{
			int ROBspace;
			if (R_Buffer.T < R_Buffer.H)
				ROBspace = R_Buffer.H - R_Buffer.T;
			else if (R_Buffer.H < R_Buffer.T)
				ROBspace = R_Buffer.SIZE - (R_Buffer.T - R_Buffer.H);
			else
			{
				if (R_Buffer.T < (R_Buffer.SIZE - 1))
				{
					if (R_Buffer.rob[R_Buffer.T + 1].dest == 0 && R_Buffer.rob[R_Buffer.T + 1].pc == 0 && R_Buffer.rob[R_Buffer.T + 1].Isready == 0)
						ROBspace = R_Buffer.SIZE;
					else
						ROBspace = 0;
				}
				else
				{
					if (R_Buffer.rob[R_Buffer.T - 1].dest == 0 && R_Buffer.rob[R_Buffer.T - 1].pc == 0 && R_Buffer.rob[R_Buffer.T - 1].Isready == 0)
						ROBspace = R_Buffer.SIZE;
					else
						ROBspace = 0;
				}
			}

			if ((unsigned)ROBspace < RN.reg.size())
				return;
			else
			{
				for (unsigned int i = 0; i < RN.reg.size(); i++)
				{
					if (RN.reg[i].rs1 != -1)
						if (Rename_Table.rmt[RN.reg[i].rs1].Isvalid == true)							
						{
							RN.reg[i].rs1 = Rename_Table.rmt[RN.reg[i].rs1].tag;						
							RN.reg[i].rs1_ROB = true;										
						}


					if (RN.reg[i].rs2 != -1)
						if (Rename_Table.rmt[RN.reg[i].rs2].Isvalid == true)						
						{
							RN.reg[i].rs2 = Rename_Table.rmt[RN.reg[i].rs2].tag;						
							RN.reg[i].rs2_ROB = true;										
						}

					R_Buffer.rob[R_Buffer.T].dest = RN.reg[i].dst;									
					R_Buffer.rob[R_Buffer.T].pc = RN.reg[i].pc;									
					R_Buffer.rob[R_Buffer.T].Isready = false;											
					if (RN.reg[i].dst != -1)												
					{
						Rename_Table.rmt[RN.reg[i].dst].tag = R_Buffer.T;								
						Rename_Table.rmt[RN.reg[i].dst].Isvalid = true;							
					}

					RN.reg[i].dst = R_Buffer.T;												
					if (R_Buffer.T != (R_Buffer.SIZE - 1))
						R_Buffer.T++;
					else
						R_Buffer.T = 0;

					RN.reg[i].RR_start = Cycle + 1;
					RN.reg[i].RN_cycles = RN.reg[i].RR_start - RN.reg[i].RN_start;
				}
				RN.reg.swap(RR.reg);
				RN.reg.clear();
				RN.Isempty = true;
				RR.Isempty = false;
			}
		}

	}
}

void RegRead()
{
	if (!RR.Isempty)
	{
		if (DI.Isempty)
		{
			for (unsigned int i = 0; i < RR.reg.size(); i++)
			{
				if (RR.reg[i].rs1_ROB)
				{
					if (R_Buffer.rob[RR.reg[i].rs1].Isready == 1)
						RR.reg[i].rs1_rdy = true;
				}
				else
					RR.reg[i].rs1_rdy = true;
					
				if (RR.reg[i].rs2_ROB)
				{
					if (R_Buffer.rob[RR.reg[i].rs2].Isready == 1)
						RR.reg[i].rs2_rdy = true;
				}
				else
					RR.reg[i].rs2_rdy = true;

				RR.reg[i].DI_start = Cycle + 1;
				RR.reg[i].RR_cycles = RR.reg[i].DI_start - RR.reg[i].RR_start;
			}
			RR.reg.swap(DI.reg);
			RR.reg.clear();
			RR.Isempty = true;
			DI.Isempty = false;
		}
	}
}

void Dispatch() {
    if (DI.Isempty) {
        return;
    }

    size_t availableSpace = I_Queue.SIZE - I_Queue.iq.size();
    if (availableSpace >= DI.reg.size()) {
        for (size_t i = 0; i < DI.reg.size(); ++i) {
            Instruction& instr = DI.reg[i];
            instr.IS_start = Cycle + 1;
            instr.DI_cycles = instr.IS_start - instr.DI_start;
            I_Queue.iq.push_back(instr);
        }
        DI.reg.clear();
        DI.Isempty = true;
    }
}

void Issue() {
    if (I_Queue.iq.empty()) {
        return;
    }

    std::sort(I_Queue.iq.begin(), I_Queue.iq.end());

    int issuedCount = 0;
    for (std::vector<Instruction>::iterator it = I_Queue.iq.begin(); it != I_Queue.iq.end();) {
        if (issuedCount >= Width) {
            break;
        }

        if (it->rs1_rdy && it->rs2_rdy) {
            it->EX_start = Cycle + 1;
            it->IS_cycles = it->EX_start - it->IS_start;
            Exec_List.exec_list.push_back(*it);
            it = I_Queue.iq.erase(it);
            issuedCount++;
        } else {
            ++it;
        }
    }
}


void ExecuteInstruction(Instruction& command)
{
    command.WB_start = Cycle + 1;
    command.EX_cycles = command.WB_start - command.EX_start;
    WB.reg.push_back(command);
}

void WakeUpDependentInstructions(std::vector<Instruction>& queue, const int dst)
{
    for (unsigned int i = 0; i < queue.size(); i++)
    {
        if (queue[i].rs1 == dst)
            queue[i].rs1_rdy = true;

        if (queue[i].rs2 == dst)
            queue[i].rs2_rdy = true;
    }
}

void Execute()
{
    if (Exec_List.exec_list.empty())
    {
        return; 
    }

    Exec_List.dec_count(); 
    int counter = 1;

    while (counter != 0)
    {
        counter = 0;

        for (unsigned int i = 0; i < Exec_List.exec_list.size();)
        {
            if (Exec_List.exec_list[i].count == 0)
            {
                ExecuteInstruction(Exec_List.exec_list[i]);
                WakeUpDependentInstructions(I_Queue.iq, Exec_List.exec_list[i].dst);
                WakeUpDependentInstructions(DI.reg, Exec_List.exec_list[i].dst);
                WakeUpDependentInstructions(RR.reg, Exec_List.exec_list[i].dst);

                Exec_List.exec_list.erase(Exec_List.exec_list.begin() + i);
                counter++;
            }
            else
            {
                i++;
            }
        }
    }
}

void ProcessWritebackEntry(Instruction& Inst) {
    int dstIndex = Inst.dst;
    R_Buffer.rob[dstIndex].Isready = true;
    Inst.RT_start = Cycle + 1;
    Inst.WB_cycles = Inst.RT_start - Inst.WB_start;
    RT.reg.push_back(Inst);
}

void Writeback() {
    if (!WB.reg.empty()) {
        std::vector<Instruction> tempReg = WB.reg;
        for (unsigned int i = 0; i < tempReg.size(); ++i) {
            ProcessWritebackEntry(tempReg[i]);
        }
        WB.reg.clear();
    }
}

void UpdateReadyStatusInRR(PipeLine_Reg &RR, int robHead) {
    for (unsigned int j = 0; j < RR.reg.size(); j++) {
        if (RR.reg[j].rs1 == robHead)
            RR.reg[j].rs1_rdy = true;
        if (RR.reg[j].rs2 == robHead)
            RR.reg[j].rs2_rdy = true;
    }
}

void PrintAndRemoveFromRT(PipeLine_Reg &RT, int pc, int cycle) {
    for (vector<Inst>::iterator it = RT.reg.begin(); it != RT.reg.end(); ++it) {
        if (it->pc == pc) {
            it->RT_cycles = (cycle + 1) - it->RT_start;

            cout << it->pc << " fu{" << it->type << "} src{" << it->rs1_og << "," << it->rs2_og << "} ";
            cout << "dst{" << it->dst_org << "} FE{" << it->FE_start << "," << it->FE_cycles << "} ";
            cout << "DE{" << it->DE_start << "," << it->DE_cycles << "} RN{" << it->RN_start << "," << it->RN_cycles << "} ";
            cout << "RR{" << it->RR_start << "," << it->RR_cycles << "} DI{" << it->DI_start << "," << it->DI_cycles << "} ";
            cout << "IS{" << it->IS_start << "," << it->IS_cycles << "} EX{" << it->EX_start << "," << it->EX_cycles << "} ";
            cout << "WB{" << it->WB_start << "," << it->WB_cycles << "} RT{" << it->RT_start << "," << it->RT_cycles << "} " << endl;

            RT.reg.erase(it);
            break;
        }
    }
}

void UpdateRMTAndClearROB(Rename_Map_Table_Class &Rename_Table, Reorder_Buffer_Class &R_Buffer, int robHead) {
    for (int z = 0; z < Rename_Table.SIZE; z++) {
        if (Rename_Table.rmt[z].tag == robHead) {
            Rename_Table.rmt[z].tag = 0;
            Rename_Table.rmt[z].Isvalid = false;
        }
    }
    R_Buffer.rob[robHead].clear();
}

void MoveROBHead(Reorder_Buffer_Class &R_Buffer) {
    if (R_Buffer.H != (R_Buffer.SIZE - 1))
        R_Buffer.H++;
    else
        R_Buffer.H = 0;
}

void Retire() {
    int i = 0;

    while (i < Width) {
        if (R_Buffer.H == R_Buffer.T && R_Buffer.H != R_Buffer.SIZE - 1) {
            if (R_Buffer.rob[R_Buffer.H + 1].pc == 0)
                return;
        }

        if (R_Buffer.rob[R_Buffer.H].Isready) {
            UpdateReadyStatusInRR(RR, R_Buffer.H);
            PrintAndRemoveFromRT(RT, R_Buffer.rob[R_Buffer.H].pc, Cycle);
            UpdateRMTAndClearROB(Rename_Table, R_Buffer, R_Buffer.H);
            MoveROBHead(R_Buffer);
            i++;
        } else {
            break;
        }
    }
}

bool Advance_Cycle(fstream &fin){
	return !(pipeLine_empty && fin.eof());
}


int main(int argc, char *argv[])
{

	Cycle = 0;
	PC = 0;
	pipeLine_empty = false;

	proc_params params;
	params.rob_size = atoi(argv[1]);
	params.iq_size = atoi(argv[2]);
	Width = atoi(argv[3]);
	char *trace = (char *)malloc(20);
	trace = argv[4];

	R_Buffer.SIZE = params.rob_size;
	I_Queue.SIZE = params.iq_size;

	DE.Isempty = RN.Isempty = RR.Isempty = DI.Isempty = WB.Isempty = RT.Isempty = true;

	R_Buffer.H = R_Buffer.T = 3;
	Rename_Table.SIZE = 67;



	ROBInput rb;
	rb.clear();
	for (int i = 0; i < params.rob_size; i++)
		R_Buffer.rob.push_back(rb);

	for (int i = 0; i < 67; i++)
	{
		Rename_Table.rmt[i].Isvalid = false;
		Rename_Table.rmt[i].tag = -1;
	}

	fstream fin;
	FILE *pFile;

	pFile = fopen(trace, "r");
	fin.open(trace, ios::in);

	while (Advance_Cycle(fin))
	{
		Retire();
		Writeback();
		Execute();
		Issue();
		Dispatch();
		RegRead();
		Rename();
		Decode();
		Fetch(fin);

		if (DE.Isempty && RN.Isempty && RR.Isempty && DI.Isempty && I_Queue.iq.size() == 0 && Exec_List.exec_list.size() == 0 && WB.reg.size() == 0)
			if (R_Buffer.H == R_Buffer.T)
				if (R_Buffer.rob[R_Buffer.T].pc == 0)
					pipeLine_empty = true;

		Cycle++;
	}

	fclose(pFile);

	cout << "# === Simulator Command =========" << endl;
	cout << "# ";
	for (int i = 0; i < argc; i++)
		cout << argv[i] << " ";
	cout << endl
		 << "# === Processor Configuration ===" << endl;
	cout << "# ROB_SIZE = " << params.rob_size << endl;
	cout << "# IQ_SIZE  = " << params.iq_size << endl;
	cout << "# WIDTH    = " << Width << endl;
	cout << "# === Simulation Results ========" << endl;
	cout << "# Dynamic Instruction Count    = " << PC << endl;
	cout << "# Cycles                       = " << Cycle << endl;
	cout << "# Instructions Per Cycle (IPC)  = " << fixed << setprecision(2) << ((float)PC / (float)Cycle) << endl;
}