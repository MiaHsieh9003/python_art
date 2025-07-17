from art_wrapper import ARTWrapper
from pathlib import Path
workloads = ['a', 'b', 'c', "d", 'e', 'f']
sizes = [8, 16, 32, 64]
origin_method = False

def excute(art, instructions, method):
    print(f"######### {method} ########")
    if method == "ART-improve":
        origin_method = False
        for instruction in instructions:
            operation = instruction.split("||||")
            if operation[0] == "INSERT" or operation[0] == "UPDATE":
                print(f"[{operation[0]}]")
                art.insert(operation[1].replace("\n", ""), operation[2].replace("\n", ""), origin_method)
            elif operation[0] == "READ":
                print("[READ]")
                art.query(operation[1].replace("\n", ""))

    elif method == "ART-origin":
        origin_method = True
        for instruction in instructions:
            operation = instruction.split("||||")
            if operation[0] == "INSERT" or operation[0] == "UPDATE":
                # print(f"[{operation[0]}]")
                art.insert(operation[1].replace("\n", ""), operation[2].replace("\n", ""), origin_method)
            elif operation[0] == "READ":
                # print("[READ]")
                art.query(operation[1].replace("\n", ""))
    art.get_latency_energy()

def dataset_loader(workload):
    current_path = Path(__file__)
    dataset_path = current_path.parents[2] / "datasets" #current_path.parents[2]: means upper 2 parent folder
    with open(dataset_path / f"load_{workload}") as file:
        load_instructions = file.readlines()
    with open(dataset_path / f"run_{workload}") as file:
        run_instructions = file.readlines()
    # print("load_instructions: ",len(load_instructions))
    # print("run_instructions: ", len(run_instructions))
    instructions = load_instructions + run_instructions
    return instructions

workload_size = 8
workload_type = "a"
# for workload_type in workloads:
    #for workload_size in sizes:
dataset = f"workload{workload_type}_{workload_size}"
instructions = dataset_loader(dataset)
print(f"{dataset} Loaded\tInstructions:{len(instructions)}")

art = ARTWrapper()
# excute(art, instructions, "ART-improve")
excute(art, instructions, "ART-origin")