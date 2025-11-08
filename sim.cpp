#include "include/common.h"
#include "include/insmem.h"
#include "include/datamem.h"
#include "include/registerfile.h"
#include "include/core.h"
#include <cstdio>  // for std::remove

// Function to extract testcase name from path
string extractTestcaseName(const string& path) {
    // Find the last occurrence of '/' or '\'
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos) {
        string dirname = path.substr(lastSlash + 1);
        // Check if it's a testcase directory
        if (dirname.find("testcase") == 0) {
            return dirname;
        }
    }
    
    // Fallback: look for testcase in the path
    size_t pos = path.find("testcase");
    if (pos != string::npos) {
        // Extract testcase name (assume format like "testcase0", "testcase1", etc.)
        size_t start = pos;
        size_t end = start + 8; // "testcase" length
        while (end < path.length() && isdigit(path[end])) {
            end++;
        }
        return path.substr(start, end - start);
    }
    
    return "default"; // fallback name
}

int main(int argc, char* argv[]) {
	
	string ioDir = "";
    if (argc == 1) {
        cout << "Enter path containing the memory files: ";
        cin >> ioDir;
    }
    else if (argc > 2) {
        cout << "Invalid number of arguments. Machine stopped." << endl;
        return -1;
    }
    else {
        ioDir = argv[1];
        cout << "IO Directory: " << ioDir << endl;
    }

    InsMem imem = InsMem("Imem", ioDir);
    DataMem dmem_ss = DataMem("SS", ioDir);
	DataMem dmem_fs = DataMem("FS", ioDir);

    // Extract testcase name and create result subdirectory
    string testcaseName = extractTestcaseName(ioDir);
    string resultDir = "result/" + testcaseName;
    
    cout << "Testcase: " << testcaseName << endl;
    cout << "Result directory: " << resultDir << endl;

	SingleStageCore SSCore(ioDir, imem, dmem_ss);
	FiveStageCore FSCore(ioDir, imem, dmem_fs);

    // Set output directory for both cores
    SSCore.setOutputDirectory(resultDir);
    FSCore.setOutputDirectory(resultDir);

    while (1) {
		if (!SSCore.halted)
			SSCore.step();
		
		if (!FSCore.halted)
			FSCore.step();

		if (SSCore.halted && FSCore.halted)
			break;
    }
    
	// dump both data memories to result directory
	dmem_ss.outputDataMem(resultDir);
	dmem_fs.outputDataMem(resultDir);

    // Clear the performance metrics file and output for both cores
    string perfFile = resultDir + "/PerformanceMetrics.txt";
    std::remove(perfFile.c_str());  // Remove existing file to start fresh
    
    // Output performance metrics for both cores
    SSCore.outputPerformanceMetrics(resultDir);
    FSCore.outputPerformanceMetrics(resultDir);

	return 0;
}