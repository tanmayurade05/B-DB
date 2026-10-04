# B+ Tree in C

A small in-memory B+ tree written in C. It stores student records
(id, name, age, marks) keyed by `id`, and a simple menu-driven
terminal interface.

- Order 5 (max 4 keys per node, min 2)
- Leaves are linked, so range queries and sorted listing just walk the chain
- Operations: insert, search, update, delete, range search
- Duplicate IDs are rejected
- No file storage

## Build and run

    gcc bptree.c -o bptree
    ./bptree

## Usage

The program shows a menu, type the number and press Enter.

    1. Insert                 add a record (asks for id, name, age, marks)
    2. Search                 look up a record by id
    3. Update                 replace name/age/marks of an existing id
    4. Delete                 remove a record by id
    5. Range search           list all records with id between two values
    6. Show all (sorted)      list everything in key order
    7. Print tree structure   show the nodes, one per line, indented by level
    8. Load sample data       insert 12 records so the tree has a few levels
    0. Exit