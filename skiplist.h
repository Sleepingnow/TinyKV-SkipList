#include <cstring>
#include <fstream>
#include <sstream>
#include <mutex>

std::mutex mtx;     
std::string delimiter = ":";

// 跳表结点数据结构
template<typename K, typename V>
class Node
{
public:
    Node() {}

    Node(int);

    Node(K, V, int);

    ~Node();

    K get_key() const {
        return key;
    }

    V get_value() const {
        return value;
    }

    void set_value(const V v) {
        value = v;
    }

    // 结点在每一层的后继结点数组
    Node<K, V> **next;

    // 结点的层数
    int node_level;

private:
    K key;
    V value;
};

template<typename K, typename V>
Node<K, V>::Node(const int level)
{
    // key、value默认初始化
    K k;
    V v;
    key = k;
    value = v;
    node_level = level;

    next = new Node<K, V>*[level + 1];
    memset(next, 0, sizeof(Node<K, V>*)*(level + 1));
}

template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, const int level)
{
    key = k;
    value = v;
    node_level = level;

    next = new Node<K, V>*[level + 1];
    memset(next, 0, sizeof(Node<K, V>*)*(level + 1));
}

template<typename K, typename V>
Node<K, V>::~Node()
{
    delete[] next;
}



// 跳表数据结构
template<typename K, typename V>
class SkipList
{
public:
    SkipList(int);
    
    Node<K, V>* create_node(K, V);
    
    bool insert_elem(K, V);
    
    bool delete_elem(K);
    
    bool update_elem(K, V);

    bool search_elem(K, V&);

    int random_level();

    void store_file(std::ofstream& file_writer);

    void load_file(std::ifstream& file_reader);

    void display_skiplist();

    int size() {return elem_count;}

    ~SkipList();
private:
    void get_key_value_from_string(const std::string&, std::string*, std::string*);

    bool string_is_valid(const std::string&);

private:
    // 允许的最大层数
    int max_level;

    // 当前层数
    int cur_level;

    // 当前跳表中的结点个数
    int elem_count;

    // 跳表的头结点，其中不携带KV信息
    Node<K, V>* header;

    // 写入文件流
    // 读取文件流
};

template<typename K, typename V>
SkipList<K, V>::SkipList(const int alloc_max_level)
{
    max_level = alloc_max_level;
    cur_level = 0;
    elem_count = 0;
    header = new Node<K, V>(max_level);
}

template<typename K, typename V>
SkipList<K, V>::~SkipList()
{
    Node<K, V>* node = header -> next[0];
    Node<K, V>* temp;
    while(node) {
        temp = node -> next[0];
        delete node;
        node = temp;
    }
    delete header;
}

// 用抛硬币的方式获取随机层数
template<typename K, typename V>
int SkipList<K, V>::random_level()
{
    int level = 0;
    while(random() % 2 && level < max_level) 
        ++level;
    return level;
}

template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K key, const V value)
{
    int level = random_level();
    Node<K, V>* node = new Node<K, V>(key, value, level);
    return node;
}

// 查找key对应的键值对，值保存到value中
// 查找成功时返回true，失败时返回false
template<typename K, typename V>
bool SkipList<K, V>::search_elem(const K key, V& value)
{
    Node<K, V>* cur = header;

    // 从当前的顶层开始寻找
    for(int i = cur_level; i >= 0; --i) {
        // 找到cur在每一层的前驱结点
        while(cur -> next[i] != NULL && cur -> next[i] -> get_key() < key)
            cur = cur -> next[i];
    }

    if(cur -> next[0] && cur -> next[0] -> get_key() == key) {
        value = cur -> next[0] -> get_value();
        return true;
    }

    return false;
}

template<typename K, typename V>
bool SkipList<K, V>::insert_elem(const K key, const V value)
{
    mtx.lock();
    V temp;
    // key已存在于跳表中，return false
    if(search_elem(key, temp)) {
        mtx.unlock();
        return false;
    }

    Node<K, V>* cur = header;
    Node<K, V>* pre_node[max_level + 1];
    memset(pre_node, 0, sizeof(Node<K, V>*)*(max_level+1));

    // 找到新结点在每一层的前驱，从当前的最高层开始
    for(int i = cur_level; i >= 0; --i){
        while(cur -> next[i] != NULL && cur -> next[i] -> get_key() < key)
            cur = cur -> next[i];
        pre_node[i] = cur;
    }  

    Node<K, V>* node = create_node(key, value);
        
    // 新结点层数比cur_level高，则比cur_level高的部分前驱结点为header
    if(node -> node_level > cur_level) {
        for(int i = cur_level + 1; i <= node -> node_level; ++i)
            pre_node[i] = header;
        cur_level = node -> node_level;
    }

    for(int i = 0; i <= cur_level; ++i) {
        node -> next[i] = pre_node[i] -> next[i];
        pre_node[i] -> next[i] = node;
    }
    ++elem_count;

    mtx.unlock();
    return true;
}

// 删除key对应的键值对
// 删除成功时返回true，失败时返回false
template<typename K, typename V>
bool SkipList<K, V>::delete_elem(const K key) 
{
    mtx.lock();
    V temp;

    // key不存在于跳表中，return false
    if(!search_elem(key, temp)) {
        mtx.unlock();
        return false;
    }

    Node<K, V>* cur = header;
    Node<K, V>* pre_node[max_level + 1];
    memset(pre_node, 0, sizeof(Node<K, V>*)*(max_level+1));

    // 找到新结点在每一层的前驱，从当前的最高层开始
    for(int i = cur_level; i >= 0; --i){
        while(cur -> next[i] != NULL && cur -> next[i] -> get_key() < key)
            cur = cur -> next[i];
        pre_node[i] = cur;
    }  

    // 逐层删除结点
    cur = cur -> next[0];
    for(int i = cur -> node_level; i >= 0; --i)
        pre_node[i] -> next[i] = cur -> next[i];
    
    // 去除空层
    while(max_level > 0 && header -> next[max_level] == 0)
        --max_level;
    
    delete cur;
    --elem_count;
    mtx.unlock();
    return true;
}

// 更新key对应的键值对
// 更新成功时返回true，失败时返回false
template<typename K, typename V>
bool SkipList<K, V>::update_elem(const K key, const V value) 
{
    mtx.lock();
    V temp;
    // key不存在于链表中，更新失败
    if(!search_elem(key, temp)) {
        mtx.unlock();
        return false;
    }

    // key对应的值已经等于value
    if(temp == value) {
        mtx.unlock();
        return true;
    }

    Node<K, V>* cur = header;

    // 从当前的顶层开始寻找
    for(int i = cur_level; i >= 0; --i) {
        // 找到cur在每一层的前驱结点
        while(cur -> next[i] != NULL && cur -> next[i] -> get_key() < key)
            cur = cur -> next[i];
    }

    cur = cur -> next[0];
    cur -> set_value(value);

    mtx.unlock();
    return true;
} 

// 显示整个跳表结构
template<typename K, typename V> 
void SkipList<K, V>::display_skiplist() 
{
    std::cout << "\n*****SkipList*****"<<"\n"; 
    for (int i = 0; i <= cur_level; i++) {
        Node<K, V> *node = this->header->next[i]; 
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->next[i];
        }
        std::cout << std::endl;
    }
}

// 将跳表中数据存储到文件中
template<typename K, typename V> 
void SkipList<K, V>::store_file(std::ofstream& file_writer) 
{
    mtx.lock();
    
    Node<K, V>* node = header -> next[0];
    while (node != NULL)
    {
        file_writer << node -> get_key() << ":" << node -> get_value() << "\n";
        node = node -> next[0];
    }
    file_writer.flush();

    mtx.unlock();
}

// 将文件中数据加载成为一个跳表
template<typename K, typename V> 
void SkipList<K, V>::load_file(std::ifstream& file_reader) 
{
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(file_reader, line))
    {
        get_key_value_from_string(line, key, value);
        if(key -> empty() || value -> empty())
            continue;

        K k;
        V v;
        std::stringstream ss;
        ss << *key;
        ss >> k;
        ss.clear();
        ss << *value;
        ss >> v;
        insert_elem(k, v);
    }
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) 
{
    if(!string_is_valid(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::string_is_valid(const std::string& str) 
{
    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}