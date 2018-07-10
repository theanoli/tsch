import sys

def compute_throughput(total_cycles, baseline_cycles, newproto_cycles, req_rate):
	total_cycles_per_request = total_cycles/req_rate
	baseline_cpu = baseline_cycles/total_cycles_per_request
	newproto_cpu = newproto_cycles/total_cycles_per_request
	cycles_improvement = (baseline_cpu - newproto_cpu) * total_cycles
	expected_impact_cycles = cycles_improvement/total_cycles_per_request
	expected_impact_cpu = expected_impact_cycles/req_rate * 100
	print(expected_impact_cpu)

if __name__ == "__main__":
	compute_throughput(int(sys.argv[1]), int(sys.argv[2]), 
			int(sys.argv[3]), int(sys.argv[4]))
