# QuackQuack - Build a Tiny Computer in C

Welcome! This repository contains the **QuackQuack** project: you will build a tiny computer in C and run a maze game inside it.

You have not learned C yet - that is expected. The labs will teach you C step-by-step.

---

## What is in this project?

- `quack.c`  
  **The only file you edit.** Contains the whole simulator (memory + CPU + I/O).

- `programs/`  
  Pre-compiled “machine code” programs (`*.duck`) that run on your Quack computer:
  - `sum10.duck`
  - `call_add.duck`
  - `maze.duck`

---

## Before you start: what is compiling?

C is a **compiled** language.

- You write source code (`.c`)
- A compiler turns it into an executable program (here: `quack`)

That executable is what you run in the terminal.

---

## What is `make`?

`make` is a build tool. It reads a file called `Makefile`.

Our `Makefile` tells `make`:

- how to compile `quack.c`
- which compiler flags to use
- how to clean up compiled files

When you run:

```bash
make
```

It runs a command similar to:

```bash
gcc -std=c11 -Wall -Wextra -O2 -o quack quack.c
```

**Meaning of flags:**
- `-std=c11` : use the C11 standard
- `-Wall -Wextra` : show useful warnings (these help you!)
- `-O2` : enable some optimization (not required, but fine)

---

# Setup instructions (Windows / macOS / Linux)

Choose your operating system and follow the steps.

---

## Windows (recommended: WSL)

The smoothest way to do C on Windows is **WSL** (Windows Subsystem for Linux).
This gives you a real Linux terminal with gcc and make.

### Step 1 - Install WSL + Ubuntu
1. Open **PowerShell as Administrator**
2. Run:

```powershell
wsl --install
```

3. Restart if asked
4. Open Ubuntu from the Start Menu and finish setup (create username/password)

### Step 2 - Install build tools inside Ubuntu
In the Ubuntu terminal:

```bash
sudo apt update
sudo apt install build-essential
```

This installs:
- `gcc` (C compiler)
- `make` (build tool)

### Step 3 - Build Quack
Go to your project folder (in WSL) and run:

```bash
make
```

### Step 4 - Run a program
```bash
./quack run programs/sum10.duck
```

---

## Windows (alternative: MSYS2) - optional
If you cannot use WSL, you can use **MSYS2** (more annoying, but possible).
If you want this path, ask us for the exact steps (WSL is preferred).

---

## macOS

### Step 1 - Install Xcode Command Line Tools
Open Terminal and run:

```bash
xcode-select --install
```

This installs `clang` (a C compiler) and `make`.

### Step 2 - Build
In the project folder:

```bash
make
```

### Step 3 - Run
```bash
./quack run programs/sum10.duck
```

---

## Linux (Ubuntu/Debian)

### Step 1 - Install compiler + make
```bash
sudo apt update
sudo apt install build-essential
```

### Step 2 - Build
```bash
make
```

### Step 3 - Run
```bash
./quack run programs/sum10.duck
```

---

# How to run programs

## 1) sum10 (quick sanity test)
```bash
./quack run programs/sum10.duck
```

Expected final value:
- `R0(final)=0037`

Note: This is **hexadecimal**.  
`0x0037 = 55` decimal, and `1+2+...+10 = 55`.

---

## 2) call_add (tests CALL/RET + stack)
```bash
./quack run programs/call_add.duck
```

---

## 3) maze (the game)
```bash
./quack run programs/maze.duck
```

### Controls
Type one key and press Enter:
- `W` = up
- `A` = left
- `S` = down
- `D` = right
- `Q` = quit

Goal: reach the exit `E`.  
Player is `@`, walls are `#`.

---

## Debug mode (very useful)
Debug mode prints the CPU state every step:

```bash
./quack run --debug programs/sum10.duck
```

This helps you see if `pc` is moving correctly and whether the program loops.

---

## Scripted input (for deterministic tests)
Instead of typing, you can provide a script of keys:

```bash
./quack run --script "DDSSAAQ" programs/maze.duck
```

This is useful for debugging and for consistent testing and helps us automatically test your code.

---

# Troubleshooting

## “command not found: make” or “gcc not found”
You don’t have build tools installed.
Follow the setup steps above for your OS.

## “Permission denied” when running ./quack
Run:

```bash
chmod +x quack
```

(Usually not needed, but sometimes happens if files moved oddly.)

## It compiles, but maze does not move
In Lab 4 you will implement the I/O reads/writes and byte load/store instructions.
Until then, maze will not be interactive.
