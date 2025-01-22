import argparse
import subprocess
import time
import os
from time import sleep

# paths to exes
ase_server_path = os.environ["ASE_SERVER_PATH"]+"ase_server"
benchmark_path = os.environ["ASE_BENCH_PATH"]

output_path = "test.txt"

benches = [
    "bench_dma", "bench_redis", "bench_tcp2", "bench_tcp2_buff", "bench_tcp2_bat",
    "bench_dma_parallel", "bench_redis_parallel", "bench_tcp2_parallel", "bench_tcp2_buff_parallel", "bench_tcp2_bat_parallel"]
start_ase_server_f = [
    False, False, True, True, True,
    False, False, True, True, True]
start_redis_server_f = [
    False, True, False, False, False,
    False, True, False, False, False]

ase_server_process = None
redis_server_process = None

def get_bench_path(name):
    return benchmark_path+name

def print_info(name):
    print(f"> {name}")

def start_redis_server():
    global redis_server_process

    if redis_server_process and redis_server_process.poll() is None:
        stop_redis_server()

    # Start the background program and return its handler
    redis_server_process = subprocess.Popen(["redis-server", "--save \"\"", "--appendonly no", "--dbfilename \"\""], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print_info("Starting redis-server...")
    sleep(2.5)
    print_info("redis-server started!")
    return

def stop_redis_server():
    global redis_server_process
    if redis_server_process and redis_server_process.poll() is None:
        print_info("Stopping redis-server...")
        redis_server_process.terminate()
        redis_server_process.wait()
        redis_server_process = None
        print_info("redis-server stopped!")
    return

def start_ase_server():
    global ase_server_process

    if ase_server_process and ase_server_process.poll() is None:
        stop_ase_server()

    # Start the background program and return its handler
    ase_server_process = subprocess.Popen([ase_server_path, "-p 42069", "--destroy"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print_info("Starting ase_server...")
    sleep(2.5)
    print_info("ase_server started!")
    return

def stop_ase_server():
    global ase_server_process
    if ase_server_process and ase_server_process.poll() is None:
        print_info("Stopping ase_server...")
        ase_server_process.terminate()
        ase_server_process.wait()
        ase_server_process = None
        print_info("ase_server stopped!")
    return

def execute_benchmarks(cycles):
    for cycle in range(0, cycles):
        print_info(f"CYCLE: {cycle}")
        for x in range(0, len(benches)):
            bench = benches[x]
            exe_path = get_bench_path(bench)
            print_info(f"Running bench '{bench}'...")
            print_info(f"path: {exe_path}")

            if start_ase_server_f[x]:
                start_ase_server()
            if start_redis_server_f[x]:
                start_redis_server()

            try:
                subprocess.run([exe_path, output_path])
                print_info(f"{bench} executed successfully!")
            except subprocess.CalledProcessError as e:
                print(f"Error occurred while running {bench}: {e}")

            if start_ase_server_f[x]:
                stop_ase_server()
            if start_redis_server_f[x]:
                stop_redis_server()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Executes all benchmarks for a defined amount of times."
    )

    parser.add_argument(
        "-o", "--output",
        type=str,
        required=True,
        help="Path to the output file."
    )
    parser.add_argument(
        "-c", "--cycles",
        type=int,
        required=True,
        help="Cycles count."
    )

    args = parser.parse_args()

    output_path = args.output
    print(f"Output file specified: {output_path}")
    print(f"cycles  : {args.cycles}")

    execute_benchmarks(args.cycles)