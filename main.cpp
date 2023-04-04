#include <iostream>
#include <vector>
#include <cstdlib>
#include <omp.h>
#include<time.h>


#define _RAND_ 0
#define _INCR_ 1
#define _OPENMP_RAND_ 2
#define _OPENMP_INCR_ 3
#define _IMPROVED_RAND_ 4
using namespace std;


//https://www.cl.cam.ac.uk/research/srg/netos/papers/2001-caslists.pdf implementation v1 04/04/2023

template <typename KeyType> class Node {
public:
    KeyType key;
    Node *next;

    Node() {}
    Node (KeyType key) {
        this->key = key;
    }

};

template <typename KeyType> class List {
public:
    Node<KeyType> *head;
    Node<KeyType> *tail;

    List() {
        head = new Node<KeyType> ();                    //An instance of the List class contains two fields which identify the head and
        tail = new Node<KeyType> ();                       //the tail. Instances of Node contain two fields identifying the key and successor of the
        head->next = tail;                                           //node.
        tail->next = nullptr;
    };
    void print() {
        Node<KeyType> *cur_node=this->head->next;
        cout<<"head->";
        do{

            cout<<cur_node->key<<"->";
    cur_node=cur_node->next;
        }while (cur_node->next!=nullptr);
            cout<<"tail"<<endl<<endl;
    }

    void print_first_n(int num){
        int i =0;
     Node<KeyType> *cur_node=this->head->next;
        cout<<"head->";
        do{
            i++;
            cout<<cur_node->key<<"->";
    cur_node=cur_node->next;
        }while (cur_node->next!=nullptr&&i<num);
            cout<<"stop"<<endl<<endl;
    }


    int get_size(){
        int j = 0;
    Node<KeyType> *cur_node=this->head->next;
        do{
            j++;
    cur_node=cur_node->next;
        }while (cur_node->next!=nullptr);
        return j;
    }

    void print_size(){
    cout<<endl<<"this list has "<<this->get_size()<<" nodes"<<endl;
    }
//static_cast<Node<KeyType>*>() -- ???
    bool is_marked_reference(Node<KeyType> *p) {
        return (uint64_t)p & 0x1;
    }

    Node<KeyType> * get_unmarked_reference(Node<KeyType> *p) {
        //  Node<KeyType> *p_temp = p&0xFF;
        return (Node<KeyType>*)( (uint64_t)p & ~0x1);
    }

    Node<KeyType> * get_marked_reference( Node<KeyType> *p) {
        return (Node<KeyType>*)( (uint64_t)p | 0x1);
    }

    bool MyInsert (KeyType key) {

        Node<KeyType> *new_node = new Node<KeyType>(key);                   //The List::MyInsert method attempts to insert a new node with the supplied
                                                                                                                    //key.
        Node<KeyType> *right_node, *left_node;

        do {
            right_node = MySearch (key, &left_node);                                        //The List::search operation finds the left and right nodes for a particular
                                                                                                                   //search key.

            if ((right_node != tail) && (right_node->key == key))           //T1
                return false;

            new_node->next = right_node;

            if (__sync_bool_compare_and_swap(&(left_node->next), right_node, new_node)) //C2              __sync_bool_compare_and_swap(addr,o,n) is a CAS operation that atomically compares the
                                                                                                                                                                //contents of addr against the old value o and if they match writes n to that
                                                                                                                                                                    // location. __sync_bool_compare_and_swap returns a boolean indicating whether this update took place.
                return true;
        } while (true); //B3

    //List::MyInsert uses List::MySearch to locate the pair of nodes between which
    //the new node is to be inserted. The update itself takes place with a single CAS
    //operation (C2) which swings the reference in left node.next from right node
    //to the new node.
    }

    bool MyDelete (KeyType search_key) {

        Node<KeyType> *right_node, *right_node_next, *left_node;                                                    //The List::MyDelete method attempts to remove a node containing the supplied
                                                                                                                                                        //key.


        do {

            right_node = MySearch (search_key, &left_node);

            if ((right_node == tail) || (right_node->key != search_key)) //T1
                return false;

            right_node_next = right_node->next;

            if (!is_marked_reference(right_node_next))                                                                      //The reference contained in the next field of a node may be in one of two
                                                                                                                                                    //states: marked or unmarked.
                if (__sync_bool_compare_and_swap(&(right_node->next), //C3                                     Intuitively a marked node
                    right_node_next, get_marked_reference (right_node_next)))                                      //is one which should be ignored because some process is deleting it. The function
                                                                                                                                                 //is marked reference(r) returns true if and only if r is a marked reference.
                                                                                                                                                 //Similarly get_marked_reference(r) and get_unmarked_reference(r) convert
                                                                                                                                                 //between marked and unmarked references.

                    break;
        } while (true); //B4

        if (!__sync_bool_compare_and_swap(&(left_node->next), right_node, right_node_next)) //C4
            right_node = MySearch (right_node->key, &left_node);
        return true;

    //List::MyDelete uses List::MySearch to locate the node to delete and then uses
    //a two-stage process to perform the deletion. Firstly, the node is logically deleted
    //by marking the reference contained in right node.next (C3). Secondly, the node
    //is physically deleted. This may be performed directly (C4) or within a separate
    //invocation of MySearch.
    }

    bool MyFind (KeyType search_key) {

        Node<KeyType>*right_node, *left_node;                                                                   //The List::MyFind method tests whether the list contains a node with
                                                                                                                        //the supplied key
        right_node = MySearch (search_key, &left_node);

        if ((right_node == tail) ||
                (right_node->key != search_key))
            return false;
        else
            return true;
    }
private:
    Node<KeyType> *MySearch (KeyType search_key, Node <KeyType>**left_node) {

    Node<KeyType> *left_node_next, *right_node;
                                                                                                            //List::MySearch takes a search key and returns references to two
search_again:                                                                                        //nodes called the left node and right node for that key. The method ensures that
        do {                                                                                               //these nodes satisfy a number of conditions. Firstly, the key of the left node must
            Node<KeyType> *t = head;                                                                         //be less than the search key and the key of the right node must be greater than
            Node<KeyType> *t_next = head->next;                                                         //or equal to the search key. Secondly, both nodes must be unmarked. Finally, the
                                                                                                                //right node must be the immediate successor of the left node.
            // 1: Find left_node and right_node
            do {
                if (!is_marked_reference(t_next)) {
                    (*left_node) = t;
                    left_node_next = t_next;
                }
                t = get_unmarked_reference(t_next);
                if (t == tail)
                    break;
                t_next = t->next;
            } while (is_marked_reference(t_next) || (t->key<search_key)); //B1
            right_node = t;
            // 2: Check nodes are adjacent
            if (left_node_next == right_node)
                if ((right_node != tail) && is_marked_reference(right_node->next))
                    goto search_again;//G1
                else
                    return right_node;//R1
            else{}
            // 3: Remove one or more marked nodes
            if (__sync_bool_compare_and_swap(&(*left_node)->next, left_node_next, right_node)) //C1
                if ((right_node != tail) && is_marked_reference(right_node->next)){
                    goto search_again;} //G2
                else
                    return right_node; //R2
            else{}
        } while (true); //B2

    //The List::MyFind invokes List::MySearch and
    //examines the resulting right node.

    }

};
/*
public sealed class LcmRandom : Random          //prng
{
    // fields
    int cur;

    public LcmRandom(int a, int c, int m, int divisor = 1, int remeinder = 0x7fff_ffff)
    {
        // init
        cur = 1;
    }
    public LcmRandom(int divisor = 1, int remeinder = 0x4000_0000) :
        this(1103515245, 12345, 0x4000_0000, divisor, remainder) { } // gcc impl

    public override int Next()
    {
        // checks

        cur = (cur * a + c) % m;

        return (cur / divisor) % remainder;
    }
}
*/
void print_vec(vector<List<int>> vec){
for(std::vector<List<int>>::iterator iter = vec.begin(); iter != vec.end(); ++iter) {
    iter->print();
 }

 }
void init_vec(vector<List<int>> *vec, int list_num, int list_size, int init_param) {
         int j = 0;
    switch(init_param) {
    case 0://random %100+1

        for(int i =0; i<list_num; i++) {
            List<int> MyList;
            for (int k = 0; k<list_size; k++)
                MyList.MyInsert(rand());
            vec->push_back(MyList);
        }
        break;
    case 1://increment

        for(int i =0; i<list_num; i++) {
            List<int> MyList;
            for (int k = 0; k<list_size; k++)
                MyList.MyInsert(++j);
            vec->push_back(MyList);
        }
        break;
    case 2://openmp random - seems fine
        for(int i =0; i<list_num; i++) {
            List<int> MyList;
            #pragma omp parallel for schedule(static) num_threads(2)
            for (int k = 0; k<list_size; k++)
                MyList.MyInsert(rand());
            vec->push_back(MyList);
        }
        break;
    case 3://openmp increment -- to improve!

         for(int i =0; i<list_num; i++) {
            List<int> MyList;
            #pragma omp parallel for schedule(static) num_threads(10) //private(j)
            for (int k = 0; k<list_size; k++)
                MyList.MyInsert(++j);
            vec->push_back(MyList);
        }
        break;
    case 4://improved random?
        break;
    default:
        break;
    }
}


int main() {
    int num_lists = 1;
    int lists_size = 250000;
    int print_first_n = 100;
    vector<List<int>> container;
    init_vec(&container, num_lists, lists_size, _OPENMP_RAND_);    //_RAND_    _INCR_    _OPENMP_RAND_    _OPENMP_INCR_    _IMPROVED_RAND_
      //  print_vec(container);
        container[0].print_size();
        container[0].print_first_n(print_first_n);

    //todo std::list
    return 0;
}
