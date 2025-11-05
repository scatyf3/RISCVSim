#include "include/common.h"
#include "include/insmem.h"
#include "include/datamem.h"
#include "include/registerfile.h"

// Standard headers
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>  // 新增：用于创建输出目录

using std::string;
using std::ofstream;
using std::cout;
using std::endl;
using std::uint32_t;

class Core {
    public:
        RegisterFile myRF;
        uint32_t cycle = 0;
        bool halted = false;
        string ioDir;
        struct stateStruct state, nextState;
        InsMem ext_imem;
        DataMem ext_dmem;
        
        Core(string ioDir, InsMem &imem, DataMem &dmem): myRF(ioDir), ioDir{ioDir}, ext_imem {imem}, ext_dmem {dmem} {}

        virtual void step() {}

        virtual void printState() {}

        virtual void setOutputDirectory(const string& outputDir) {
            if (outputDir.empty()) return;
            ioDir = outputDir;
            std::error_code ec;
            std::filesystem::create_directories(ioDir, ec); 
        }
};

class SingleStageCore : public Core {
    public:
        SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "\\SS_", imem, dmem), opFilePath(ioDir + "\\StateResult_SS.txt") {}

        // 新增：覆盖，设置 SS 的状态输出文件
        void setOutputDirectory(const string& outputDir) override {
            Core::setOutputDirectory(outputDir);
            opFilePath = ioDir + "/StateResult_SS.txt";
        }

        void step() {
            /* Your implementation*/

            halted = true;
            if (state.IF.nop)
                halted = true;
            
            myRF.outputRF(cycle); // dump RF
            printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
            
            state = nextState; // The end of the cycle and updates the current state with the values calculated in this cycle
            cycle++;
        }

        void printState(stateStruct state, int cycle) {
    		ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else 
    			printstate.open(opFilePath, std::ios_base::app);
    		if (printstate.is_open()) {
    		    printstate<<"State after executing cycle:\t"<<cycle<<endl; 

    		    printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;
    		    printstate<<"IF.nop:\t"<<state.IF.nop<<endl;
    		}
    		else cout<<"Unable to open SS StateResult output file." << endl;
    		printstate.close();
		}
	private:
		string opFilePath;
};

class FiveStageCore : public Core{
    public:
        
        FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "\\FS_", imem, dmem), opFilePath(ioDir + "\\StateResult_FS.txt") {}

        void setOutputDirectory(const string& outputDir) override {
            Core::setOutputDirectory(outputDir);
            opFilePath = ioDir + "/StateResult_FS.txt";
        }

        void step() {
            /* Your implementation */
            /* --------------------- WB stage --------------------- */
			
			
			
			/* --------------------- MEM stage -------------------- */
			
			
			
			/* --------------------- EX stage --------------------- */
			
			
			
			/* --------------------- ID stage --------------------- */
			
			
			
			/* --------------------- IF stage --------------------- */
			
			
			halted = true;
			if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop)
				halted = true;
        
            myRF.outputRF(cycle); // dump RF
			printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
       
			state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
			cycle++;
		}

		void printState(stateStruct state, int cycle) {
		    ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else 
		    	printstate.open(opFilePath, std::ios_base::app);
		    if (printstate.is_open()) {
		        printstate<<"State after executing cycle:\t"<<cycle<<endl; 

		        printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;        
		        printstate<<"IF.nop:\t"<<state.IF.nop<<endl; 

		        printstate<<"ID.Instr:\t"<<state.ID.Instr<<endl; 
		        printstate<<"ID.nop:\t"<<state.ID.nop<<endl;

		        printstate<<"EX.Read_data1:\t"<<state.EX.Read_data1<<endl;
		        printstate<<"EX.Read_data2:\t"<<state.EX.Read_data2<<endl;
		        printstate<<"EX.Imm:\t"<<state.EX.Imm<<endl; 
		        printstate<<"EX.Rs:\t"<<state.EX.Rs<<endl;
		        printstate<<"EX.Rt:\t"<<state.EX.Rt<<endl;
		        printstate<<"EX.Wrt_reg_addr:\t"<<state.EX.Wrt_reg_addr<<endl;
		        printstate<<"EX.is_I_type:\t"<<state.EX.is_I_type<<endl; 
		        printstate<<"EX.rd_mem:\t"<<state.EX.rd_mem<<endl;
		        printstate<<"EX.wrt_mem:\t"<<state.EX.wrt_mem<<endl;        
		        printstate<<"EX.alu_op:\t"<<state.EX.alu_op<<endl;
		        printstate<<"EX.wrt_enable:\t"<<state.EX.wrt_enable<<endl;
		        printstate<<"EX.nop:\t"<<state.EX.nop<<endl;        

		        printstate<<"MEM.ALUresult:\t"<<state.MEM.ALUresult<<endl;
		        printstate<<"MEM.Store_data:\t"<<state.MEM.Store_data<<endl; 
		        printstate<<"MEM.Rs:\t"<<state.MEM.Rs<<endl;
		        printstate<<"MEM.Rt:\t"<<state.MEM.Rt<<endl;   
		        printstate<<"MEM.Wrt_reg_addr:\t"<<state.MEM.Wrt_reg_addr<<endl;              
		        printstate<<"MEM.rd_mem:\t"<<state.MEM.rd_mem<<endl;
		        printstate<<"MEM.wrt_mem:\t"<<state.MEM.wrt_mem<<endl; 
		        printstate<<"MEM.wrt_enable:\t"<<state.MEM.wrt_enable<<endl;         
		        printstate<<"MEM.nop:\t"<<state.MEM.nop<<endl;        

		        printstate<<"WB.Wrt_data:\t"<<state.WB.Wrt_data<<endl;
		        printstate<<"WB.Rs:\t"<<state.WB.Rs<<endl;
		        printstate<<"WB.Rt:\t"<<state.WB.Rt<<endl;
		        printstate<<"WB.Wrt_reg_addr:\t"<<state.WB.Wrt_reg_addr<<endl;
		        printstate<<"WB.wrt_enable:\t"<<state.WB.wrt_enable<<endl;
		        printstate<<"WB.nop:\t"<<state.WB.nop<<endl; 
		    }
		    else cout<<"Unable to open FS StateResult output file." << endl;
		    printstate.close();
		}
	private:
		string opFilePath;
};