import os

predictors = ["LocalBP", "BiModeBP", "PerceptronBP"]

benchmarks_dir = os.path.join(os.curdir, "microbench")

control_benchmarks = ["CCa", "CCe", "CCh", "CCh_st", "CCm", "CCl", "CRd", "CRf", "CS1", "CS3"]

out_dir = "out"

for benchmark in control_benchmarks:
        workload_path = os.path.join(benchmarks_dir, benchmark, "bench")
        for predictor in predictors:
            cmd = f"./build/X86/gem5.opt --outdir={out_dir} --stats-file={f'{benchmark}-{predictor}-stats.txt'} --dump-config={f'{benchmark}-{predictor}-config.ini'} --json-config={f'{benchmark}-{predictor}-config.json'} configs/deprecated/example/se.py --cmd={workload_path} --bp-type={predictor}"
            os.system(cmd)
