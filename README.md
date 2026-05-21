# CTF-Tubes-IF2224-2026


### Deskripsi

Repository ini dibuat untuk memenuhi Tugas Besar Mata Kuliah Teori Bahasa Formal dan Otomata (IF2224) 2025/2026. Proyek ini merupakan implementasi **Arion Compiler** yang akan dikembangkan secara bertahap.

#### **Milestone 1: Lexical Analysis**

Tahap pertama berfokus pada pengembangan **Lexical Analyzer** yang berfungsi untuk melakukan tokenisasi kode sumber bahasa Arion menggunakan pendekatan Deterministic Finite Automaton (DFA).

**Komponen yang diimplementasikan:**
1. **DFA (Deterministic Finite Automaton)** - Mesin state untuk pengenalan pola token
2. **Lexer** - Penganalisis leksikal yang mengubah kode sumber menjadi stream of tokens
3. **Token Generator** - Pembangkit token berdasarkan aturan DFA

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

#### Lexer mode

```bash
make lexer
```

```bash
make lexer test/milestone-1/input/input-1.txt
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



**Output:**
Program akan menampilkan token, parse tree, atau hasil analisis semantik sesuai mode yang digunakan.

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
        └── milestone-3
            ├── input
            │   └── *.txt
            └── output
                └── output-*.txt
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
  Laporan (Bab 1)<br/>
</td>
<td>22%</td>
</tr>
<tr>
<td>Nathaniel Christian</td>
<td>13524122</td>
<td>
  Laporan (Bab 2)<br/>
</td>
<td>22%</td>
</tr>
<tr>
<td>Azri Arzaq Pohan</td>
<td>13524139</td>
<td>
    Laporan (Bab 3 dan 4) <br/>
    Test cases
</td>
<td>21%</td>
</tr>
<tr>
<td>Rasyad Satyatma</td>
<td>13524142</td>
<td>
  <code>./src/Semantic</code><br/>
</td>
<td>35%</td>
</tr>

</table>
</div>
