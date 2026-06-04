#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string>
#include <yaml-cpp/yaml.h>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/idl/go2/LowState_.hpp>
#include <unitree/idl/go2/LowCmd_.hpp>
#include <unitree/common/time/time_tool.hpp>
#include <unitree/common/thread/thread.hpp>

using namespace unitree::common;
using namespace unitree::robot;

constexpr double PosStopF = (2.146E+9f);
constexpr double VelStopF = (16000.0f);

struct AgentConfig
{
    int domain_id = 1;
    std::string interface = "lo";
    std::string lowcmd_topic = "rt/lowcmd";
    std::string lowstate_topic = "rt/lowstate";
};

std::filesystem::path DefaultConfigPath(const char *argv0)
{
    std::filesystem::path exe_path(argv0);
    auto exe_dir = exe_path.has_parent_path() ? exe_path.parent_path() : std::filesystem::path(".");
    return exe_dir / ".." / ".." / ".." / "config.yaml";
}

AgentConfig LoadAgentConfig(const std::filesystem::path &config_path)
{
    AgentConfig config;

    try
    {
        auto yaml = YAML::LoadFile(config_path.string());
        if (yaml["dds"])
        {
            auto dds = yaml["dds"];
            if (dds["domain_id"])
            {
                config.domain_id = dds["domain_id"].as<int>();
            }
            if (dds["interface"])
            {
                config.interface = dds["interface"].as<std::string>();
            }
        }
        if (yaml["topics"])
        {
            auto topics = yaml["topics"];
            if (topics["lowcmd"])
            {
                config.lowcmd_topic = topics["lowcmd"].as<std::string>();
            }
            if (topics["lowstate"])
            {
                config.lowstate_topic = topics["lowstate"].as<std::string>();
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to load config file: " << config_path << "\n"
                  << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return config;
}

AgentConfig ParseArgs(int argc, const char **argv)
{
    auto config_path = DefaultConfigPath(argv[0]);
    bool legacy_interface_arg = false;
    std::string legacy_interface;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc)
        {
            config_path = argv[++i];
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage:\n"
                      << "  ./main [--config PATH] [--domain ID] [--interface IFACE]\n"
                      << "  ./main IFACE  # legacy: domain 0, given interface\n";
            exit(EXIT_SUCCESS);
        }
        else if (arg == "--domain" && i + 1 < argc)
        {
            // Parsed after YAML is loaded below.
            ++i;
        }
        else if (arg == "--interface" && i + 1 < argc)
        {
            // Parsed after YAML is loaded below.
            ++i;
        }
        else if (!arg.empty() && arg[0] != '-')
        {
            legacy_interface_arg = true;
            legacy_interface = arg;
        }
        else
        {
            std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    AgentConfig config = LoadAgentConfig(config_path);

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc)
        {
            ++i;
        }
        else if (arg == "--domain" && i + 1 < argc)
        {
            config.domain_id = std::stoi(argv[++i]);
        }
        else if (arg == "--interface" && i + 1 < argc)
        {
            config.interface = argv[++i];
        }
    }

    if (legacy_interface_arg)
    {
        config.domain_id = 0;
        config.interface = legacy_interface;
    }

    return config;
}

class Custom
{
public:
    Custom(const AgentConfig &config) : config_(config){};
    ~Custom(){};
    void Init();

private:
    void InitLowCmd();
    void LowStateMessageHandler(const void *messages);
    void LowCmdWrite();

private:
    double stand_up_joint_pos[12] = {0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763,
                                     0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763};
    double stand_down_joint_pos[12] = {0.0473455, 1.22187, -2.44375, -0.0473455, 1.22187, -2.44375, 0.0473455,
                                       1.22187, -2.44375, -0.0473455, 1.22187, -2.44375};
    double dt = 0.002;
    double runing_time = 0.0;
    double phase = 0.0;
    AgentConfig config_;

    unitree_go::msg::dds_::LowCmd_ low_cmd{};     // default init
    unitree_go::msg::dds_::LowState_ low_state{}; // default init

    /*publisher*/
    ChannelPublisherPtr<unitree_go::msg::dds_::LowCmd_> lowcmd_publisher;
    /*subscriber*/
    ChannelSubscriberPtr<unitree_go::msg::dds_::LowState_> lowstate_subscriber;

    /*LowCmd write thread*/
    ThreadPtr lowCmdWriteThreadPtr;
};

uint32_t crc32_core(uint32_t *ptr, uint32_t len)
{
    unsigned int xbit = 0;
    unsigned int data = 0;
    unsigned int CRC32 = 0xFFFFFFFF;
    const unsigned int dwPolynomial = 0x04c11db7;

    for (unsigned int i = 0; i < len; i++)
    {
        xbit = 1 << 31;
        data = ptr[i];
        for (unsigned int bits = 0; bits < 32; bits++)
        {
            if (CRC32 & 0x80000000)
            {
                CRC32 <<= 1;
                CRC32 ^= dwPolynomial;
            }
            else
            {
                CRC32 <<= 1;
            }

            if (data & xbit)
                CRC32 ^= dwPolynomial;
            xbit >>= 1;
        }
    }

    return CRC32;
}

void Custom::Init()
{
    InitLowCmd();
    /*create publisher*/
    lowcmd_publisher.reset(new ChannelPublisher<unitree_go::msg::dds_::LowCmd_>(config_.lowcmd_topic));
    lowcmd_publisher->InitChannel();

    /*create subscriber*/
    lowstate_subscriber.reset(new ChannelSubscriber<unitree_go::msg::dds_::LowState_>(config_.lowstate_topic));
    lowstate_subscriber->InitChannel(std::bind(&Custom::LowStateMessageHandler, this, std::placeholders::_1), 1);

    /*loop publishing thread*/
    lowCmdWriteThreadPtr = CreateRecurrentThreadEx("writebasiccmd", UT_CPU_ID_NONE, int(dt * 1000000), &Custom::LowCmdWrite, this);
}

void Custom::InitLowCmd()
{
    low_cmd.head()[0] = 0xFE;
    low_cmd.head()[1] = 0xEF;
    low_cmd.level_flag() = 0xFF;
    low_cmd.gpio() = 0;

    for (int i = 0; i < 20; i++)
    {
        low_cmd.motor_cmd()[i].mode() = (0x01); // motor switch to servo (PMSM) mode
        low_cmd.motor_cmd()[i].q() = (PosStopF);
        low_cmd.motor_cmd()[i].kp() = (0);
        low_cmd.motor_cmd()[i].dq() = (VelStopF);
        low_cmd.motor_cmd()[i].kd() = (0);
        low_cmd.motor_cmd()[i].tau() = (0);
    }
}

void Custom::LowStateMessageHandler(const void *message)
{
    low_state = *(unitree_go::msg::dds_::LowState_ *)message;
}

void Custom::LowCmdWrite()
{

    runing_time += dt;
    if (runing_time < 3.0)
    {
        // Stand up in first 3 second

        // Total time for standing up or standing down is about 1.2s
        phase = tanh(runing_time / 1.2);
        for (int i = 0; i < 12; i++)
        {
            low_cmd.motor_cmd()[i].q() = phase * stand_up_joint_pos[i] + (1 - phase) * stand_down_joint_pos[i];
            low_cmd.motor_cmd()[i].dq() = 0;
            low_cmd.motor_cmd()[i].kp() = phase * 50.0 + (1 - phase) * 20.0;
            low_cmd.motor_cmd()[i].kd() = 3.5;
            low_cmd.motor_cmd()[i].tau() = 0;
        }
    }
    else
    {
        // Then stand down
        phase = tanh((runing_time - 3.0) / 1.2);
        for (int i = 0; i < 12; i++)
        {
            low_cmd.motor_cmd()[i].q() = phase * stand_down_joint_pos[i] + (1 - phase) * stand_up_joint_pos[i];
            low_cmd.motor_cmd()[i].dq() = 0;
            low_cmd.motor_cmd()[i].kp() = 50;
            low_cmd.motor_cmd()[i].kd() = 3.5;
            low_cmd.motor_cmd()[i].tau() = 0;
        }
    }

    low_cmd.crc() = crc32_core((uint32_t *)&low_cmd, (sizeof(unitree_go::msg::dds_::LowCmd_) >> 2) - 1);
    lowcmd_publisher->Write(low_cmd);
}

int main(int argc, const char **argv)
{
    AgentConfig config = ParseArgs(argc, argv);
    ChannelFactory::Instance()->Init(config.domain_id, config.interface);
    std::cout << "DDS domain: " << config.domain_id
              << ", interface: " << config.interface
              << ", lowcmd topic: " << config.lowcmd_topic
              << ", lowstate topic: " << config.lowstate_topic << std::endl;

    std::cout << "Press enter to start";
    std::cin.get();
    Custom custom(config);
    custom.Init();

    while (1)
    {
        sleep(10);
    }

    return 0;
}
