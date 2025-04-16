if __name__ == "__main__":
    instruction_set = set()
    f = open("pokemon.hex", "r")
    for line in f:
        arr = line.split(":")
        if not (arr[0] in instruction_set):
            instruction_set.add(arr[0])
    f.close()
    f = open("instructions.txt", "w")
    for instruction in instruction_set:
        f.write(instruction + "\n")
    f.close()