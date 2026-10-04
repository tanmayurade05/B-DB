#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 5
#define MAX_KEYS (ORDER - 1)
#define MIN_KEYS (MAX_KEYS / 2)

typedef struct 
{
    int id;
    char name[32];
    int age;
    float marks;
} Student;

typedef struct Node 
{
    int is_leaf;
    int n;
    int keys[MAX_KEYS + 1];          // one spare slot so a node can overflow before splitting
    struct Node *child[MAX_KEYS + 2];
    Student data[MAX_KEYS + 1];      // only used by leaves
    struct Node *next;               // leaf chain
} Node;

static Node *root = NULL;

static Node *new_node(int leaf)
{
    Node *n = calloc(1, sizeof(Node));

    if(!n) 
    {
        printf("out of memory\n");
        exit(1);
    }

    n->is_leaf = leaf;
    return n;
}

static Node *find_leaf(int key)
{
    Node *cur = root;

    while(cur && !cur->is_leaf) 
    {
        int i = 0;

        while(i < cur->n && key >= cur->keys[i])
        i++;

        cur = cur->child[i];
    }

    return cur;
}

static Student *search(int key)
{
    Node *leaf = find_leaf(key);

    if(!leaf)
    return NULL;

    for(int i = 0; i < leaf->n; i++)
        if(leaf->keys[i] == key)
        return &leaf->data[i];

    return NULL;
}

// returns 1 if the node split; up_key/up_node are then set for the parent
static int insert_rec(Node *node, Student s, int *up_key, Node **up_node)
{
    if(node->is_leaf) 
    {
        int i = node->n;

        while(i > 0 && node->keys[i - 1] > s.id) 
        {
            node->keys[i] = node->keys[i - 1];
            node->data[i] = node->data[i - 1];
            i--;
        }

        node->keys[i] = s.id;
        node->data[i] = s;
        node->n++;

        if(node->n <= MAX_KEYS)
        return 0;

        Node *right = new_node(1);
        int left_n = (node->n + 1) / 2;
        right->n = node->n - left_n;

        for(int j = 0; j < right->n; j++) 
        {
            right->keys[j] = node->keys[left_n + j];
            right->data[j] = node->data[left_n + j];
        }

        node->n = left_n;
        right->next = node->next;
        node->next = right;

        *up_key = right->keys[0];
        *up_node = right;

        return 1;
    }

    int i = 0;
    while(i < node->n && s.id >= node->keys[i])
    i++;

    int k;
    Node *nn;
    if(!insert_rec(node->child[i], s, &k, &nn))
    return 0;

    for(int j = node->n; j > i; j--) 
    {
        node->keys[j] = node->keys[j - 1];
        node->child[j + 1] = node->child[j];
    }

    node->keys[i] = k;
    node->child[i + 1] = nn;
    node->n++;

    if(node->n <= MAX_KEYS)
    return 0;

    int mid = node->n / 2;
    Node *right = new_node(0);
    right->n = node->n - mid - 1;

    for(int j = 0; j < right->n; j++)
    right->keys[j] = node->keys[mid + 1 + j];

    for(int j = 0; j <= right->n; j++)
    right->child[j] = node->child[mid + 1 + j];

    *up_key = node->keys[mid];
    node->n = mid;
    *up_node = right;

    return 1;
}

static int insert(Student s)
{
    if(search(s.id))
    return 0;

    if(!root)
    root = new_node(1);

    int k;
    Node *nn;
    if(insert_rec(root, s, &k, &nn)) 
    {
        Node *r = new_node(0);
        r->n = 1;
        r->keys[0] = k;
        r->child[0] = root;
        r->child[1] = nn;
        root = r;
    }

    return 1;
}

static void borrow_from_left(Node *p, int i)
{
    Node *c = p->child[i];
    Node *l = p->child[i - 1];

    for(int j = c->n; j > 0; j--) 
    {
        c->keys[j] = c->keys[j - 1];

        if(c->is_leaf)
        c->data[j] = c->data[j - 1];
    }

    if(!c->is_leaf)
        for(int j = c->n + 1; j > 0; j--)
        c->child[j] = c->child[j - 1];

    if(c->is_leaf) 
    {
        c->keys[0] = l->keys[l->n - 1];
        c->data[0] = l->data[l->n - 1];
        p->keys[i - 1] = c->keys[0];
    } 
    
    else 
    {
        c->keys[0] = p->keys[i - 1];
        c->child[0] = l->child[l->n];
        p->keys[i - 1] = l->keys[l->n - 1];
    }

    l->n--;
    c->n++;
}

static void borrow_from_right(Node *p, int i)
{
    Node *c = p->child[i];
    Node *r = p->child[i + 1];

    if(c->is_leaf) 
    {
        c->keys[c->n] = r->keys[0];
        c->data[c->n] = r->data[0];

        for(int j = 0; j < r->n - 1; j++) 
        {
            r->keys[j] = r->keys[j + 1];
            r->data[j] = r->data[j + 1];
        }

        p->keys[i] = r->keys[0];
    } 
    
    else 
    {
        c->keys[c->n] = p->keys[i];
        c->child[c->n + 1] = r->child[0];
        p->keys[i] = r->keys[0];

        for(int j = 0; j < r->n - 1; j++)
        r->keys[j] = r->keys[j + 1];

        for(int j = 0; j < r->n; j++)
        r->child[j] = r->child[j + 1];
    }

    c->n++;
    r->n--;
}

// merges child idx + 1 into child idx and drops the separator from the parent
static void merge_children(Node *p, int idx)
{
    Node *l = p->child[idx];
    Node *r = p->child[idx + 1];

    if(l->is_leaf) 
    {
        for(int j = 0; j < r->n; j++) 
        {
            l->keys[l->n + j] = r->keys[j];
            l->data[l->n + j] = r->data[j];
        }

        l->n += r->n;
        l->next = r->next;
    } 
    
    else 
    {
        l->keys[l->n] = p->keys[idx];
        l->n++;

        for(int j = 0; j < r->n; j++)
        l->keys[l->n + j] = r->keys[j];

        for(int j = 0; j <= r->n; j++)
        l->child[l->n + j] = r->child[j];

        l->n += r->n;
    }

    for(int j = idx; j < p->n - 1; j++)
    p->keys[j] = p->keys[j + 1];

    for(int j = idx + 1; j < p->n; j++)
    p->child[j] = p->child[j + 1];

    p->n--;
    free(r);
}

static void fix_child(Node *p, int i)
{
    if(i > 0 && p->child[i - 1]->n > MIN_KEYS)
    borrow_from_left(p, i);

    else if(i < p->n && p->child[i + 1]->n > MIN_KEYS)
    borrow_from_right(p, i);

    else if(i > 0)
    merge_children(p, i - 1);

    else
    merge_children(p, i);
}

static int delete_rec(Node *node, int key)
{
    if(node->is_leaf) 
    {
        int i = 0;
        while(i < node->n && node->keys[i] != key)
        i++;

        if(i == node->n)
        return 0;

        for(; i < node->n - 1; i++) 
        {
            node->keys[i] = node->keys[i + 1];
            node->data[i] = node->data[i + 1];
        }

        node->n--;
        return 1;
    }

    int i = 0;
    while(i < node->n && key >= node->keys[i])
    i++;

    if(!delete_rec(node->child[i], key))
    return 0;

    if(node->child[i]->n < MIN_KEYS)
    fix_child(node, i);

    return 1;
}

static int delete_key(int key)
{
    if(!root || !delete_rec(root, key))
    return 0;

    if(root->n == 0) 
    {
        Node *old = root;
        root = old->is_leaf ? NULL : old->child[0];
        free(old);
    }

    return 1;
}

static int update(Student s)
{
    Student *p = search(s.id);

    if(!p)
    return 0;

    *p = s;
    return 1;
}

static void print_student(const Student *s)
{
    printf("  %-6d %-20s %-4d %.1f\n", s->id, s->name, s->age, s->marks);
}

static void print_header()
{
    printf("  %-6s %-20s %-4s %s\n", "ID", "Name", "Age", "Marks");
}

static int range_search(int lo, int hi)
{
    if(lo > hi) 
    {
        int t = lo;
        lo = hi;
        hi = t;
    }

    Node *leaf = find_leaf(lo);
    int count = 0, done = 0;

    while(leaf && !done) 
    {
        for(int i = 0; i < leaf->n; i++) 
        {
            if(leaf->keys[i] > hi) 
            {
                done = 1;
                break;
            }

            if(leaf->keys[i] >= lo) 
            {
                if(count == 0)
                print_header();

                print_student(&leaf->data[i]);
                count++;
            }
        }

        leaf = leaf->next;
    }

    return count;
}

static int show_all()
{
    Node *leaf = root;
    while(leaf && !leaf->is_leaf)
    leaf = leaf->child[0];

    int count = 0;
    for(; leaf; leaf = leaf->next) 
    {
        for(int i = 0; i < leaf->n; i++) 
        {
            if(count == 0)
            print_header();

            print_student(&leaf->data[i]);
            count++;
        }
    }

    return count;
}

static void print_node(Node *node, int depth)
{
    printf("%*s[", depth * 4, "");

    for(int i = 0; i < node->n; i++)
    printf(i ? " %d" : "%d", node->keys[i]);

    printf("]%s\n", node->is_leaf ? " leaf" : "");

    if(!node->is_leaf)
        for(int i = 0; i <= node->n; i++)
        print_node(node->child[i], depth + 1);
}

static void free_tree(Node *node)
{
    if(!node)
    return;

    if(!node->is_leaf)
        for(int i = 0; i <= node->n; i++)
        free_tree(node->child[i]);

    free(node);
}

static void read_line(char *buf, int size)
{
    if(!fgets(buf, size, stdin)) 
    {
        printf("\n");
        free_tree(root);
        exit(0);
    }

    buf[strcspn(buf, "\n")] = '\0';
}

static int ask_int(const char *msg)
{
    char line[64];
    while(1) 
    {
        printf("%s", msg);
        read_line(line, sizeof line);
        char *end;
        long v = strtol(line, &end, 10);

        if(end != line && *end == '\0')
        return (int)v;

        printf("Please enter a whole number.\n");
    }
}

static float ask_float(const char *msg)
{
    char line[64];
    while(1) 
    {
        printf("%s", msg);
        read_line(line, sizeof line);
        char *end;
        float v = strtof(line, &end);

        if(end != line && *end == '\0')
        return v;

        printf("Please enter a number.\n");
    }
}

static Student ask_student(int id)
{
    Student s = {0};
    char line[64];

    s.id = id;
    printf("Name: ");
    read_line(line, sizeof line);
    strncpy(s.name, line, sizeof s.name - 1);

    s.age = ask_int("Age: ");
    s.marks = ask_float("Marks: ");
    return s;
}

static void do_insert()
{
    int id = ask_int("ID: ");
    if(search(id)) 
    {
        printf("ID %d already exists.\n", id);
        return;
    }

    insert(ask_student(id));
    printf("Inserted.\n");
}

static void do_search()
{
    Student *s = search(ask_int("ID to search: "));
    if(!s) 
    {
        printf("Not found.\n");
        return;
    }

    print_header();
    print_student(s);
}

static void do_update()
{
    int id = ask_int("ID to update: ");
    if(!search(id)) 
    {
        printf("Not found.\n");
        return;
    }

    printf("Enter the new details.\n");
    update(ask_student(id));
    printf("Updated.\n");
}

static void do_delete()
{
    if(delete_key(ask_int("ID to delete: ")))
    printf("Deleted.\n");

    else
    printf("Not found.\n");
}

static void do_range()
{
    int lo = ask_int("From ID: ");
    int hi = ask_int("To ID: ");
    int count = range_search(lo, hi);

    if(count == 0)
    printf("No records in that range.\n");

    else
    printf("%d record(s) found.\n", count);
}

static void do_show_all()
{
    if(show_all() == 0)
    printf("Tree is empty.\n");
}

static void do_print_tree()
{
    if(!root)
    printf("Tree is empty.\n");

    else
    print_node(root, 0);
}

static void load_sample()
{
    Student sample[] = 
    {
        {10, "Aarav", 20, 78.5f},  {20, "Isha", 21, 85.0f},
        {5, "Rohan", 19, 66.0f},   {15, "Meera", 22, 91.5f},
        {25, "Kabir", 20, 72.0f},  {30, "Diya", 21, 88.0f},
        {1, "Neel", 19, 59.5f},    {12, "Sana", 20, 81.0f},
        {18, "Varun", 23, 69.0f},  {40, "Tara", 22, 94.0f},
        {35, "Omkar", 21, 75.5f},  {8, "Priya", 20, 83.0f},
    };

    int n = sizeof sample / sizeof sample[0];
    int added = 0;

    for(int i = 0; i < n; i++)
    added += insert(sample[i]);

    printf("Loaded %d sample record(s).\n", added);
}

int main()
{
    while(1) 
    {
        printf("\n--- B+ Tree Student Records ---\n");
        printf("1. Insert\n");
        printf("2. Search\n");
        printf("3. Update\n");
        printf("4. Delete\n");
        printf("5. Range search\n");
        printf("6. Show all (sorted)\n");
        printf("7. Print tree structure\n");
        printf("8. Load sample data\n");
        printf("0. Exit\n");

        switch (ask_int("Choice: ")) 
        {
            case 1: do_insert(); break;
            case 2: do_search(); break;
            case 3: do_update(); break;
            case 4: do_delete(); break;
            case 5: do_range(); break;
            case 6: do_show_all(); break;
            case 7: do_print_tree(); break;
            case 8: load_sample(); break;

            case 0: free_tree(root); return 0;

            default: printf("Invalid choice.\n");
        }
    }
}