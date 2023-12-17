#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <vector>
#include <chrono>
#include <thread>
#include <ctime>

using namespace rapidjson;

// Function to read and parse JSON file
bool readJSONFile(const std::string &filename, Document &document)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open the JSON file." << std::endl;
        return false;
    }

    std::string jsonContent((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    document.Parse(jsonContent.c_str());

    return true;
}

// Native OS API processes
class NativeOSAPI {
public:
    void startProcess(const std::string& processName, const std::vector<std::string>& parameters) {
        std::cout << "Starting process: " << processName << std::endl;
        // Add implementation to start the process using the Native OS API
    }

    void stopProcess(const std::string& processName) {
        std::cout << "Stopping process: " << processName << std::endl;
        // Add implementation to stop the process using the Native OS API
    }

    bool isProcessRunning(const std::string& processName) {
        std::cout << "Checking if process is running: " << processName << std::endl;
        // Add implementation to check if the process is running using the Native OS API
        
        return false;
    }
};

// Eventlogger class to log the timestamp and respective event
class EventLogger {
public:
    EventLogger(const std::string& logFilePath) : logFilePath(logFilePath) {
        logFile.open(logFilePath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error opening log file: " << logFilePath << std::endl;
        }
    }

    ~EventLogger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void logEvent(const std::string& event) {
        if (logFile.is_open()) {
            auto currentTime = getCurrentTime();
            logFile << "[" << currentTime << "] " << event << std::endl;
            logFile.flush(); // Flush to ensure immediate write
        }
    }

private:
    std::string logFilePath;
    std::ofstream logFile;

    std::string getCurrentTime() const {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm currentTime = *std::localtime(&timeT);

        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &currentTime);
        return std::string(buffer);
    }
};


// ProcessManager class for start processes, monitoring proecess and RestartProcesses by checking periodically 
class ProcessManager {
public:
    ProcessManager(NativeOSAPI& osAPI, EventLogger& eventLogger)
        : osAPI(osAPI), eventLogger(eventLogger) {}

    void startMonitoring(const std::vector<std::string>& processes) {
        for (const auto& process : processes) {
            startProcess(process);
        }
    }

    void continueMonitoring(const std::vector<std::string>& processes) {
        checkAndRestartProcesses(processes);
    }

private:
    NativeOSAPI& osAPI;
    EventLogger& eventLogger;

    void startProcess(const std::string& processName) {
        if (!osAPI.isProcessRunning(processName)) {
            // Get startup parameters from configuration (not shown in this example)
            std::vector<std::string> startupParameters = getStartupParameters(processName);
            osAPI.startProcess(processName, startupParameters);
            eventLogger.logEvent("Started process: " + processName);
        }
    }

    void checkAndRestartProcesses(const std::vector<std::string>& processes) {
        for (const auto& process : processes) {
            if (!osAPI.isProcessRunning(process)) {
                // Get startup parameters from configuration (not shown in this example)
                std::vector<std::string> startupParameters = getStartupParameters(process);
                osAPI.startProcess(process, startupParameters);
                eventLogger.logEvent("Restarted process: " + process);
            }
        }
    }

    std::vector<std::string> getStartupParameters(const std::string& processName) {
        // Add implementation to retrieve startup parameters from configuration
        // This is a placeholder, replace with actual implementation
        return std::vector<std::string>();
    }
};

// ConfigurationManager to load configuration from JSON file
class ConfigurationManager
{
public:
    ConfigurationManager(const std::string &configFilePath) : configFilePath(configFilePath)
    {
        config = loadConfig();
    }

    void reloadConfiguration()
    {
        // Reload configuration dynamically
        config = loadConfig();
        if (!config.IsNull())
        {
            std::cout << "Configuration reloaded." << std::endl;
        }
    }

    std::vector<std::string> getProcesses() const
    {
        // Get the list of processes from the configuration
        const Value &processesArray = config["processes"];  // get processes
        std::vector<std::string> processes;
        for (SizeType i = 0; i < processesArray.Size(); i++)
        {
            const Value &processObject = processesArray[i];
            processes.push_back(processObject["name"].GetString()); // get name of the process
        }
        return processes;
    }

private:
    std::string configFilePath;
    Document config;

    Document loadConfig() const
    {
        try
        {
            std::ifstream file(configFilePath);
            if (!file.is_open())
            {
                throw std::runtime_error("Configuration file not found at " + configFilePath);
            }

            IStreamWrapper isw(file);
            Document configData;
            configData.ParseStream(isw);
            if (configData.HasParseError())
            {
                throw std::runtime_error("Error parsing JSON configuration");
            }

            return configData;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error loading JSON configuration: " << e.what() << std::endl;
            return Document();
        }
    }
};

int main()
{   
    // NativeOSAPI instance
    NativeOSAPI osAPI;

    // EventLogger instance
    EventLogger eventLogger("event_log.txt");
    const std::string filename = "config.json";
    // Document instance
    Document document;
    std::string previousContent;

    // ConfigurationManager instance
    ConfigurationManager configManager(filename);
    // ProcessManager instance
    ProcessManager processManager(osAPI, eventLogger);

    // log startup event
    eventLogger.logEvent("Program started.");

    // getting the processes from JSON file using configManager
    std::vector<std::string> processes = configManager.getProcesses();
    // keeping the processes into a volatile veriable
    std::vector<std::string> proceses = {processes};
    // starting processes using processManager
    processManager.startMonitoring(proceses);

    while (true)
    {   
        //debug out
        std::cout << "Processes to monitor:" << std::endl;
        //loop for printing the processes in debug log
        for (const auto &process : processes)
        {   
            std::cout << process << std::endl;
        }
        
        // Read the current content of the JSON file
        if (readJSONFile(filename, document))
        {
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            document.Accept(writer);

            // storing the json file content 
            std::string currentContent = buffer.GetString();

            // Comparing the current and previous JSON content
            if (currentContent != previousContent)
            {   
                // debug out
                std::cout << "JSON file has been modified." << std::endl;
                
                // Logging event
                eventLogger.logEvent("JSON file has been modified."); 

                // reloading configuration dynamically if change detected
                configManager.reloadConfiguration();

                // accessing processes after reloading
                std::vector<std::string> updatedProcesses = configManager.getProcesses();
                // debug out 
                std::cout << "Updated processes to monitor:" << std::endl;
                // loop for printing the updated processes in debug log
                for (const auto &process : updatedProcesses)
                {
                    std::cout << process << std::endl;
                }

                // Update the previousContent for the next iteration
                previousContent = currentContent;
            }
            else
            {   
                // No change detected
                std::cout << "JSON file has not been modified." << std::endl;
                // Logging event
                eventLogger.logEvent("JSON file has not been modified.");
            }
        }
        // keep Monitoring the state of the process's
        processManager.continueMonitoring(proceses);
        // Sleep for a 10 seconds before checking again
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}