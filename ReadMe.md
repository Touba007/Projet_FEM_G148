# 🧮 Finite Element Analysis – Mousqueton Simulation

## 🔸 Group 148

- Jacopo Visentin – NOMA: 50112100  
- Thomas Leblanc – NOMA: 83552100

---

## 📝 Project Overview

This project implements a 2D linear elasticity simulation of a **climbing carabiner (mousqueton)** using the Finite Element Method (FEM).  
The program includes mesh generation with GMSH, automatic mesh cleaning using Python, FEM solving in C, and real-time OpenGL visualization.

---

## 📁 Project Structure

```bash
/group148-jvisentin-tleblanc/
│
├── data/                         # Input/output data
│   ├── mesh_raw.txt             # Raw mesh from GMSH
│   ├── mesh_fixed.txt           # Cleaned and connected mesh
│   └── ...
│
├── headers/                      # Header files
│   ├── fem.h
│   └── glfem.h
│
├── src/                          # Source code
│   ├── fixmesh.py               # Python script to clean mesh
│   ├── fem.c                    # FEM core logic
│   ├── glfem.c                  # OpenGL visualization
│   └── run.c                    # Main program entry point
│
├── .venv/                        # Python virtual environment (excluded from Git)
├── Makefile                     # Build / run automation
├── .gitignore
└── README.md
```

---

## 🚀 How to Run the Project

### 1. 🧱 Set up Python environment

At the root of the project, create a virtual environment and install `numpy`:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install numpy
```

> This environment is used to run the Python script `fixmesh.py` which is automatically called from the `run.c` program.

### 2. 🛠️ Compile and Run

To compile the code, use : 

```bash
make build
```

To run the code with the default arguments, use : 

```bash
make run
```

You can run the code with the flags defined below using :

```bash
make run ARGS=" ... "
```


✅ `run.c` will automatically:
- Generate a GMSH mesh
- Call `fixmesh.py` using `.venv/bin/python`
- Import and solve the FEM problem
- Display the solution using OpenGL

---

## ⚙️ Available Flags

| Flag         | Description                          |
|--------------|--------------------------------------|
| `--o`        | Open carabiner                       |
| *(default)*  | Closed carabiner                     |
| `--fine`     | Smaller mesh size (0.25)             |
| `--tiny`     | Very fine mesh (0.1)                 |
| `--fweak`    | Weak downward force (2e6 N)          |
| `--fstrong`  | Strong downward force (5e6 N)        |
| `--steel`    | Use steel material                   |
| *(default)*  | Use aluminium                        |
| `--amplify`  | Amplify deformation for display      |

---