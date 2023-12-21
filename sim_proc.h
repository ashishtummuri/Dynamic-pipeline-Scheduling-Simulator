#ifndef SIM_PROC_H
#define SIM_PROC_H
#include <vector>

using namespace std;

int Cycle, PC, Width;
bool pipeLine_empty;

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct RegisterMapTable
{
	bool Isvalid;
	unsigned long int tag;
} RMTInput;

typedef struct Instruction
{
	int pc, type, dst, rs1, rs2, count;
    int dst_org, rs1_og, rs2_og;
	bool rs1_rdy, rs2_rdy, rs1_ROB, rs2_ROB;
	int FE_start, FE_cycles, DE_start, DE_cycles, RN_start, RN_cycles, RR_start, RR_cycles, DI_start, DI_cycles;
	int IS_start, IS_cycles, EX_start, EX_cycles, WB_start, WB_cycles, RT_start, RT_cycles;
	
	bool operator<(const Instruction &temp) const   
	{
		return (pc < temp.pc);
	}
} Inst;

typedef struct ReorderBuffer
{
	unsigned long int dest, pc;
	bool Isready;
	void clear()
	{
		dest = pc = 0;
		Isready = false;
	}
} ROBInput;


class PipeLine_Reg
{
public:
	bool Isempty;
	vector<Inst> reg;
};

class Issue_Queue_Class
{
public:
	unsigned long int SIZE;
	vector<Inst> iq;
};

class Reorder_Buffer_Class
{
public:
	unsigned long int H, T;
	unsigned long int SIZE;
	vector<ROBInput> rob;
};

class Rename_Map_Table_Class
{
public:
	unsigned long int SIZE;
	RMTInput rmt[67];
};

class Execution_Order
{
public:
	vector<Inst> exec_list;
	void dec_count()
	{
		for (unsigned long int i = 0; i < exec_list.size(); i++)
			exec_list[i].count--;
	}
};

PipeLine_Reg DE, RN, RR, DI, WB, RT;
Execution_Order Exec_List;
Reorder_Buffer_Class R_Buffer;
Issue_Queue_Class I_Queue;
Rename_Map_Table_Class Rename_Table;

#endif
