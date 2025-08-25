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
                # print(f"[{operation[0]}]")
                art.insert(operation[1].replace("\n", ""), operation[2].replace("\n", ""), origin_method)
            elif operation[0] == "READ":
                # print("[READ]")
                art.query(operation[1].replace("\n", ""), origin_method)

    elif method == "ART-origin":
        # print("len instruction: ", len(instructions))
        origin_method = True
        for instruction in instructions:
            operation = instruction.split("||||")

            if operation[0] == "INSERT" or operation[0] == "UPDATE":
                # print(f"[{operation[0]}]")
                art.insert(operation[1].replace("\n", ""), operation[2].replace("\n", ""), origin_method)
            elif operation[0] == "READ":
                # print("[READ]")
                art.query(operation[1].replace("\n", ""), origin_method)
                print("", end ="")
    art.get_latency_energy()
    art.get_space()

def dataset_loader(workload):
    current_path = Path(__file__)
    dataset_path = current_path.parents[2] / "datasets"  #current_path.parents[2]: means upper 2 parent folde
    
    if not (dataset_path / f"load_{workload}").exists():
        raise FileNotFoundError("Missing load file!")
    with open(dataset_path / f"load_{workload}") as file:
        load_instructions = file.readlines()
    with open(dataset_path / f"run_{workload}") as file:
        run_instructions = file.readlines()
    # print("len load: ", len(load_instructions))
    # print("len run: ", len(run_instructions))
    instructions = load_instructions + run_instructions
    return instructions

# workload_size = 8
# workload_type = "e"
for workload_type in workloads:
    for workload_size in sizes:
        dataset = f"workload{workload_type}_{workload_size}"
        print()
        print(dataset)
        instructions = dataset_loader(dataset)
        excute(ARTWrapper(), instructions.copy(), "ART-origin")
        excute(ARTWrapper(), instructions.copy(), "ART-improve")