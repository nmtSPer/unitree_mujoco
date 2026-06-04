import argparse
from pathlib import Path
import time
import sys
import numpy as np
import yaml

stand_up_joint_pos = np.array([
    0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763,
    0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763
],
                              dtype=float)

stand_down_joint_pos = np.array([
    0.0473455, 1.22187, -2.44375, -0.0473455, 1.22187, -2.44375, 0.0473455,
    1.22187, -2.44375, -0.0473455, 1.22187, -2.44375
],
                                dtype=float)

dt = 0.002
runing_time = 0.0

def default_config_path():
    return Path(__file__).resolve().parents[1] / "config.yaml"


def load_agent_config(config_path):
    with open(config_path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}

    dds = data.get("dds", {})
    topics = data.get("topics", {})
    return {
        "domain_id": int(dds.get("domain_id", 1)),
        "interface": str(dds.get("interface", "lo")),
        "lowcmd_topic": str(topics.get("lowcmd", "rt/lowcmd")),
    }


def parse_args():
    parser = argparse.ArgumentParser(
        description="Go2 stand controller using Unitree SDK2 Python channels"
    )
    parser.add_argument(
        "legacy_interface",
        nargs="?",
        help="Legacy shortcut: use DDS domain 0 with this network interface",
    )
    parser.add_argument("--config", default=str(default_config_path()))
    parser.add_argument("--domain", type=int)
    parser.add_argument("--interface")
    args = parser.parse_args()

    config = load_agent_config(args.config)
    if args.domain is not None:
        config["domain_id"] = args.domain
    if args.interface is not None:
        config["interface"] = args.interface
    if args.legacy_interface:
        config["domain_id"] = 0
        config["interface"] = args.legacy_interface

    return config

if __name__ == '__main__':
    config = parse_args()

    from unitree_sdk2py.core.channel import ChannelPublisher
    from unitree_sdk2py.core.channel import ChannelFactoryInitialize
    from unitree_sdk2py.idl.default import unitree_go_msg_dds__LowCmd_
    from unitree_sdk2py.idl.unitree_go.msg.dds_ import LowCmd_
    from unitree_sdk2py.utils.crc import CRC

    crc = CRC()
    ChannelFactoryInitialize(config["domain_id"], config["interface"])
    print(
        "DDS domain: "
        f"{config['domain_id']}, interface: {config['interface']}, "
        f"lowcmd topic: {config['lowcmd_topic']}"
    )

    input("Press enter to start")

    # Create a publisher to publish the data defined in UserData class
    pub = ChannelPublisher(config["lowcmd_topic"], LowCmd_)
    pub.Init()

    cmd = unitree_go_msg_dds__LowCmd_()
    cmd.head[0] = 0xFE
    cmd.head[1] = 0xEF
    cmd.level_flag = 0xFF
    cmd.gpio = 0
    for i in range(20):
        cmd.motor_cmd[i].mode = 0x01  # (PMSM) mode
        cmd.motor_cmd[i].q = 0.0
        cmd.motor_cmd[i].kp = 0.0
        cmd.motor_cmd[i].dq = 0.0
        cmd.motor_cmd[i].kd = 0.0
        cmd.motor_cmd[i].tau = 0.0

    while True:
        step_start = time.perf_counter()

        runing_time += dt

        if (runing_time < 3.0):
            # Stand up in first 3 second
            
            # Total time for standing up or standing down is about 1.2s
            phase = np.tanh(runing_time / 1.2)
            for i in range(12):
                cmd.motor_cmd[i].q = phase * stand_up_joint_pos[i] + (
                    1 - phase) * stand_down_joint_pos[i]
                cmd.motor_cmd[i].kp = phase * 50.0 + (1 - phase) * 20.0
                cmd.motor_cmd[i].dq = 0.0
                cmd.motor_cmd[i].kd = 3.5
                cmd.motor_cmd[i].tau = 0.0
        else:
            # Then stand down
            phase = np.tanh((runing_time - 3.0) / 1.2)
            for i in range(12):
                cmd.motor_cmd[i].q = phase * stand_down_joint_pos[i] + (
                    1 - phase) * stand_up_joint_pos[i]
                cmd.motor_cmd[i].kp = 50.0
                cmd.motor_cmd[i].dq = 0.0
                cmd.motor_cmd[i].kd = 3.5
                cmd.motor_cmd[i].tau = 0.0

        cmd.crc = crc.Crc(cmd)
        pub.Write(cmd)

        time_until_next_step = dt - (time.perf_counter() - step_start)
        if time_until_next_step > 0:
            time.sleep(time_until_next_step)
