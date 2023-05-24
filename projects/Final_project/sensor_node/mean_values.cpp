#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include <numeric>
#include <iomanip>

int main() {
    std::ifstream logFile("reading_from_sensor.txt");
    if (!logFile.is_open()) {
        std::cout << "Failed to open the log file." << std::endl;
        return 1;
    }

    std::vector<int> cpu, lpm, deep_lpm, tx, rx, off;
    std::regex permilRegex("\\((\\d+) permil\\)");
    std::smatch match;

    std::string line;
    while (std::getline(logFile, line)) {
        if (std::regex_search(line, match, permilRegex)) {
            int permilValue = std::stoi(match[1]);
            if (line.contains("CPU")) {
                cpu.push_back(permilValue);
            } else if (line.contains("Deep LPM")) {
                deep_lpm.push_back(permilValue);
            }else if (line.contains("LPM")) {
                lpm.push_back(permilValue);
            }else if (line.contains("Radio Tx")) {
                tx.push_back(permilValue);
            }else if (line.contains("Radio Rx")) {
                rx.push_back(permilValue);
            } else if (line.contains("Radio total")) {
                off.push_back(permilValue);
            }
        }
    }

    logFile.close();

    std::ofstream outputFile("mean_reading_from_sensor.txt");
    if (!outputFile.is_open()) {
        std::cout << "Failed to create the output file." << std::endl;
        return 1;
    }

    outputFile << "CPU : " << static_cast<float>(std::accumulate(cpu.begin(), cpu.end(), 0)/cpu.size())/10 << "%\n";
    outputFile << "LPM : " << static_cast<float>(std::accumulate(lpm.begin(), lpm.end(), 0)/cpu.size())/10 << "%\n";
    outputFile << "Deep LPM : " << static_cast<float>(std::accumulate(deep_lpm.begin(), deep_lpm.end(), 0)/cpu.size())/10 << "%\n";
    outputFile << "Radio Tx : " << static_cast<float>(std::accumulate(tx.begin(), tx.end(), 0)/cpu.size())/10 << "%\n";
    outputFile << "Radio Rx : " << static_cast<float>(std::accumulate(rx.begin(), rx.end(), 0)/cpu.size())/10 << "%\n";
    outputFile << "Radio Tot : " << static_cast<float>(std::accumulate(off.begin(), off.end(), 0)/off.size())/10 << "%\n";

    outputFile.close();
    std::cout << "Mean permil values have been written to the output file." << std::endl;

    return 0;
}
