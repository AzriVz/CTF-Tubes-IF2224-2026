# CTF-Tubes-IF2224-2026


### Deskripsi

Repository ini dibuat untuk memenuhi Tugas Besar Mata Kuliah Teori Bahasa Formal dan Otomata (IF2224) 2025/2026. Proyek ini merupakan implementasi **Arion Compiler** yang dikembangkan bertahap hingga mampu membangkitkan intermediate code dan menjalankannya dengan interpreter berbasis stack machine.

#### **Milestone 4: Intermediate Code and Interpreter**

Tahap akhir berfokus pada pengembangan **Intermediate Code Generator** dan **Interpreter**. Source code Arion diproses melalui lexer, parser, dan semantic analyzer, lalu Decorated AST diterjemahkan menjadi intermediate code. Intermediate code tersebut kemudian dieksekusi oleh interpreter untuk menghasilkan output program.

**Komponen yang diimplementasikan:**
1. **Lexer** - Mengubah source code menjadi stream of tokens
2. **Parser** - Membentuk parse tree berdasarkan grammar Arion
3. **Semantic Analyzer** - Menghasilkan Decorated AST dan symbol table
4. **Intermediate Code Generator** - Menerjemahkan Decorated AST menjadi instruksi intermediate code
5. **Stack Machine Interpreter** - Mengeksekusi intermediate code dan menghasilkan output program

### Requirements
- [C++ Compiler](https://gcc.gnu.org/releases.html) (C++11 compatible)
- [GNU Make](https://ftpmirror.gnu.org/make/)

### Initialization

- **Clone repository**

  ```bash
  git clone https://github.com/AzriVz/CTF-Tubes-IF2224-2026
  ```

- **Navigasi ke project directory**

  ```bash
  cd CTF-Tubes-IF2224-2026/
  ```

### Usage


```bash
make
```

#### Build dan jalankan mode Intermediate Code

Secara default, target `run` menjalankan mode Milestone 4 (`--ic`) dengan input `test/milestone-4/input/input-1.txt`.

```bash
make run
```

Untuk menjalankan file tertentu:

```bash
make run test/milestone-4/input/input-2.txt
```

Atau jalankan binary secara langsung. Secara default, pemanggilan tanpa flag akan menjalankan mode intermediate code dan interpreter.

```bash
./bin/arion-compiler test/milestone-4/input/input-1.txt
```

#### Lexer mode

```bash
make lexer
```

```bash
make lexer test/milestone-1/input/input-1.txt
```

```bash
./bin/arion-compiler --lexer test/milestone-1/input/input-1.txt
```

#### Parser mode

```bash
make parse
```

```bash
make parse test/milestone-2/input/input-1.txt
```

```bash
./bin/arion-compiler --parse test/milestone-2/input/input-1.txt
```

#### Semantic mode

```bash
make semantic
```

```bash
make semantic test/milestone-3/input/input-1.txt
```

```bash
./bin/arion-compiler --semantic test/milestone-3/input/input-1.txt
```


#### Intermediate Code mode

```bash
make ic
```

```bash
make ic test/milestone-4/input/input-1.txt
```

```bash
./bin/arion-compiler --ic test/milestone-4/input/input-1.txt
```

**Output:**
Program akan menampilkan token, parse tree, hasil analisis semantik, atau intermediate code sesuai mode yang digunakan.

---

### Struktur Folder

```
.
└── CTF-Tubes-IF2224-2026
    ├── Makefile
    ├── README.md
    ├── bin
    │   └── arion-compiler
    ├── doc
    │   ├── Laporan-1-CTF.pdf
    │   └── Laporan-2-CTF.pdf
    ├── src
    │   ├── Lexer
    │   │   ├── Lexer.cpp
    │   │   └── Lexer.hpp
    │   ├── Parser
    │   │   ├── Parser.cpp
    │   │   └── Parser.hpp
    │   ├── Semantic
    │   │   ├── AST.cpp
    │   │   ├── AST.hpp
    │   │   ├── SemanticAnalyzer.cpp
    │   │   ├── SemanticAnalyzer.hpp
    │   │   ├── SymbolTable.cpp
    │   │   └── SymbolTable.hpp
    │   ├── Intermediate
    │   │   ├── CodeGenerator.cpp
    │   │   ├── CodeGenerator.hpp
    │   │   ├── Instruction.cpp
    │   │   ├── Instruction.hpp
    │   │   ├── Interpreter.cpp
    │   │   └── Interpreter.hpp
    │   └── main.cpp
    └── test
        ├── milestone-1
        │   ├── input
        │   │   └── *.txt
        │   ├── output
        │   │   └── output-*.txt
        │   └── README.md
        ├── milestone-2
        │   ├── input
        │   │   └── *.txt
        │   └── output
        │       └── *_parse.txt
        ├── milestone-3
        │   ├── input
        │   │   └── *.txt
        │   └── output
        │       └── output-*.txt
        └── milestone-4
            ├── input
            │   └── input-*.txt
            ├── output
            │   └── input-*_ic.txt
            └── README.md
```

---

### Pembagian Tugas

<div align="center">
<table>
<tr>
<th>Nama</th>
<th>NIM</th>
<th>Workload</th>
<th>Persentase</th>
</tr>
<tr>
<td>Billy Ontoseno Irawan</td>
<td>13524121</td>
<td>
  Laporan (Bab 1-2)<br/>
</td>
<td>15%</td>
</tr>
<tr>
<td>Nathaniel Christian</td>
<td>13524122</td>
<td>
    <code>CodeGenerator.hpp</code><br/>
    <code>CodeGenerator.cpp</code><br/>
    Laporan (Bab 2,3, dan 4)<br/>
</td>
<td>35%</td>
</tr>
<tr>
<td>Azri Arzaq Pohan</td>
<td>13524139</td>
<td>
    <code>CodeGenerator.hpp</code><br/>
    <code>CodeGenerator.cpp</code><br/>
    <code>Instruction.hpp</code><br/>
    <code>Instruction.cpp</code><br/>
    <code>Interpreter.hpp</code><br/>
    <code>Interpreter.cpp</code><br/>
</td>
<td>50%</td>
</tr>
<tr>
<td>Rasyad Satyatma</td>
<td>13524142</td>
<td>

</td>
<td>0%</td>
</tr>

</table>
</div>
