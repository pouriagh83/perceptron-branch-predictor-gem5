import os
from concurrent.futures import ThreadPoolExecutor

N_THREADS = 8

executor = ThreadPoolExecutor(max_workers=N_THREADS)
futures = []

predictors = ["LocalBP", "BiModeBP"]#, "PerceptronBP"]

benchmarks_dir = os.path.join(os.curdir, "microbench")

control_benchmarks = ["CCa", "CCe", "CCh", "CCh_st", "CCm", "CCl", "CRd", "CRf", "CS1", "CS3"]

out_dir = "out"

for benchmark in control_benchmarks:
        workload_path = os.path.join(benchmarks_dir, benchmark, "bench")
        for predictor in predictors:
            cmd = f"./build/X86/gem5.opt --outdir={out_dir} --stats-file={f'{benchmark}-stats.txt'} configs/deprecated/example/se.py --cmd={workload_path} --bp-type={predictor}"
            futures.append(executor.submit(os.system, cmd))
            print(f"workload {benchmark}, bp {predictor} start")

for future in futures:
    if future.result() != 0:
        print(future.result())
        
print("completed")
