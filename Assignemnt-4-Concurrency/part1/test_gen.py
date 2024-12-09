import sys

def main():
    # Check if an argument is provided
    if len(sys.argv) != 2:
        print("Usage: python main.py <N>")
        return

    try:
        # Convert the command-line argument to an integer
        N = int(sys.argv[1])
    except ValueError:
        print("Please provide a valid integer for N.")
        return

    # Open the file in write mode
    with open("input-part1.txt", "w") as f:
        # Write numbers from 1 to N
        for i in range(1, N + 1):
            f.write(f"{i}\n")
        # Write 0 as the last line
        f.write("0\n")

    print("File 'input-part1.txt' created successfully.")

if __name__ == "__main__":
    main()
