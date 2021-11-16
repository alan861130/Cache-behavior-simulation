#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <assert.h>
#include <string.h>
#include <map>
#include <time.h>


using namespace std;

#define basic_or_advanced 1

//*****************************************************//
// Quality and Correlation
void record_bits(string s);
void calculate_quality(void);
void calculate_correlation(void);


//*****************************************************//
// Read file
void read_config(char filename[]);
void read_bench(char filename[]);

//*****************************************************//
// Cache simulation
void show_cache_content(void);
void select_best_index(int count);
void set_index(void);
int binary_to_dec(int input[]);
void create_cache(void);
int cache_simulation(void);

//*****************************************************//
// Output the simulation result
void write_output(char filename[]);

//*****************************************************//

int Address_bits, Block_size, Cache_sets, Associativity;
int offset_size;
int cache_index_size;
int Q_size;
string benchmark;

int best_miss_count;
int simulation_count;
vector<int> best_indexing_pattern;

bool testing_flag;

class set{

    public :
    set(int id, int asso): id(id) {
        // initialize the cache set
        for(int i = 0 ; i < asso ; i++){
            this->NRU_bit.push_back(1);
            this->data.push_back("0");
        }
    }

    int id;
    
    vector<bool> NRU_bit;
    vector<string> data;

    int get_replace_target(void){

        int index;
        bool found = 0;

        // find the closest block with NRU_bit = 1
        for(int i = 0 ; i < Associativity ; i++){
            if(!found && this->NRU_bit[i] == 1){
                found = 1;
                index = i;
                break;
            }
        }

        if(found){
            return index;
        }
        else{
            // set all NRU_bit to 1
            cout << "reset NRU bits because all bits in this set is 0";

            for(int i = 0 ; i < Associativity ; i++){
                this->NRU_bit[i] = 1;
            }
            // find the closest block with NRU_bit = 1
            return 0;
        }
    }

    void replace_data(int way, string new_data){
        this->data[way] = new_data;
    }

    void reset(void){
        for(int i = 0 ; i < Associativity ; i++){
            this->data[i] = "0";
            this->NRU_bit[i] = 1;
        }
    }
};

vector<string> testcase;
vector<string> tag;
vector<bool> hit_or_miss;
vector<set*> Cache;


vector< vector<int> > A; // only record indexing bits
vector<int> indexing_pattern; // based on A
vector<float> Q; // quality measurement
vector< vector<float> > C; // correlation measurement

int main(int argc, char *argv[]){

    clock_t start, finish; 
    float duration;
    start = clock();

    read_config(argv[1]);
    read_bench(argv[2]);

    create_cache();

    // set the best pattern
    testing_flag = 1;
    set_index();

    // start simulation
    testing_flag = 0;
    best_miss_count = cache_simulation();
    

    write_output(argv[3]);

    cout << "indexing pattern : ";
    for(int i = 0 ; i < indexing_pattern.size() ; i++){
        cout << indexing_pattern[i] << " ";
    }
    cout << endl;
    cout << "best miss count : " << best_miss_count << endl;
    cout << "simulation count : " << simulation_count << endl;

    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC; 
    cout << "Time duration : " << duration << endl;
    return 0;
}

void read_config(char filename[]){

    cout << "Config file : " << filename << endl;

    ifstream fin;
    fin.open(filename, ios::in);
    assert(fin);
    
    string s;
    fin >> s >> Address_bits;
    fin >> s >> Block_size;
    fin >> s >> Cache_sets;
    fin >> s >> Associativity;

    offset_size = log2(Block_size);
    cache_index_size = log2(Cache_sets);
    Q_size = Address_bits - offset_size;

    fin.close();
}

void record_bits(string s){

    vector<int> new_index;
    
    for(int i = Address_bits - offset_size - 1 ; i >= 0 ; i--){
        int k = s[i] - '0';
        new_index.push_back(k);
    }
    A.push_back(new_index);

}

void calculate_quality(void){
    
    for(int i = 0 ; i < Q_size ; i++){
        float result;
        float number_of_0 = 0, number_of_1 = 0;
        for(int j = 0 ; j < A.size() ; j++){
            (A[j][i] == 1) ? number_of_1 ++ : number_of_0 ++;
        }
        
        result = std::min(number_of_0, number_of_1) / std::max(number_of_0, number_of_1);
        Q.push_back(result);

    }
}

void calculate_correlation(void){

    for(int i = 0 ; i < Q_size ; i++){
        vector<float> correlation;
        // j < i
        for(int j = 0 ; j < i ; j++){
            correlation.push_back(C[j][i]);
        }

        // j = i
        correlation.push_back(0);

        // j > i
        for(int j = i + 1; j < Address_bits - offset_size ; j++){
            
            float result;
            float E = 0 , D = 0;
            for(int k = 0 ; k < A.size() ; k++){
                (A[k][i] == A[k][j]) ? E++ : D++;
            }
            
            result = std::min(E, D) / std::max(E, D);
            correlation.push_back(result);
        }
        C.push_back(correlation);
    }
}

void read_bench(char filename[]){
    
    cout << "bench file : " << filename << endl << endl;

    ifstream fin;
    fin.open(filename, ios::in);
    assert(fin);

    string s;
    string new_tag;
    fin >> s >> benchmark;
    while(1){
        fin >> s;
        if(s == ".end"){
            break;
        }

        // calculate the tag
        new_tag.assign(s, 0 , Address_bits - offset_size);
        
        tag.push_back(new_tag);
        record_bits(s);
        testcase.push_back(s);
    }   
    fin.close();
}

void select_best_index(int count){

    // set bound
    int bound;
    (count == 1) ? bound = 0 : bound = indexing_pattern[count - 2] ;

    // find best Q
    vector<int> best_Q;
    float max = 0;
    for(int i = bound ; i < Q_size; i++){
        
       // try every possible Q(larger than 0)
       if(Q[i] > 0){
           best_Q.push_back(i);
       }
    }
    if(best_Q.size() == 0){
        return;
    }
    
    // store current Q
    vector<float> Q_current;
    for(int i = 0 ; i < Q_size ; i++){
        Q_current.push_back(Q[i]);
    }

    for(int i = 0; i < best_Q.size() ; i++){

        indexing_pattern.push_back(best_Q[i]);

        // If count < cache_index_size
        // update Q with correlation and search the next index
        if(count < cache_index_size){

            for(int j = 0 ; j < Q_size ; j++){
                Q[j] *= C[best_Q[i]][j];
            }

            select_best_index(count + 1);

            // reset the Q for next best_Q
            for(int j = 0 ; j < Q_size ; j++){
                Q[j] = Q_current[j];
            }

        }

        // If count = cache_index_size
        // Do cache simulation with current indexing bit
        else{

            int miss_count = cache_simulation();
            simulation_count ++;

            // If this is the 1st cache simulation or the miss count is smaller than current best,
            // replace the best miss count and best indexing pattern
            if(miss_count != -1){

                if(best_miss_count == -1 || miss_count < best_miss_count){
                    best_miss_count = miss_count;

                    best_indexing_pattern.clear();
                    for(int j = 0 ; j < cache_index_size ; j++){
                        best_indexing_pattern.push_back(indexing_pattern[j]);
                        
                    }
                    
                }
            }
        }
        indexing_pattern.pop_back();
    }
}

void set_index(void){
    
    if(basic_or_advanced){

        calculate_quality();
        calculate_correlation();

        // find the best indexing pattern
        simulation_count = 0;
        best_miss_count = -1;
        select_best_index(1);

        // set indexing_pattern to the best one
        for(int i = 0 ; i < best_indexing_pattern.size() ; i++){
            indexing_pattern.push_back(best_indexing_pattern[i]);
        }

    }
    // LSB indexing
    else{
        for(int i = 0 ; i < cache_index_size ; i++){
            indexing_pattern.push_back(i);
        }
    }
}

void create_cache(void){

    for(int i = 0 ; i < Cache_sets ; i++){
        set *newset;
        newset = new set(i, Associativity);
        Cache.push_back(newset);
    }
}

void show_cache_content(void){
    for(int i = 0 ; i < Cache_sets ; i++){
        cout << "Cache set : " << i << endl;
        for(int j = 0 ; j < Associativity ; j++){
            cout << Cache[i]->data[j] << " / " << Cache[i]->NRU_bit[j] << endl;
        }
    }
}

int cache_simulation(void){

    int cache_index;
    int miss_count = 0;
    
    // reset cache
    for(int i = 0 ; i < Cache_sets ; i++){
        Cache[i]->reset();
    }
    hit_or_miss.clear();

    // go through all testcase
    for(int i = 0 ; i < testcase.size() ; i++){
        cout << i << " round " << testcase[i] << " ";
        

        // calculate the cache index 
        int k = 1;
        cache_index = 0;
        for(int j = 0 ; j < cache_index_size ; j++){
            if(A[i][indexing_pattern[j]] == 1){
                cache_index += k;
            }
            k *= 2;
        }
        
        // check the cache set 
        bool found = 0;
        for(int j = 0 ; j < Associativity ; j++){   
            if( strcmp(Cache[cache_index]->data[j].c_str() , tag[i].c_str()) == 0){

                // Reference : set NRU bit to 0
                Cache[cache_index]->NRU_bit[j] = 0;
                found = 1;
                break;
            }
        }
        
        // check hit or miss
        if(found){
            cout << "hit" << endl;
            hit_or_miss.push_back(1);
        }
        else{
            cout << "miss" << endl;
            hit_or_miss.push_back(0);
            miss_count ++;

            if(testing_flag){
                if(best_miss_count != -1 && miss_count > best_miss_count){
                    return -1;
                }
            }
            
            // get the replace target
            int replace_target = Cache[cache_index]->get_replace_target();

            // replace the target data
            //Cache[cache_index]->replace_data(replace_target, tag[i]);
            Cache[cache_index]->data[replace_target] = tag[i];

            // set the NRU bit to 0
            Cache[cache_index]->NRU_bit[replace_target] = 0;
        }

        show_cache_content();
        cout << endl;
    }
    
    return miss_count;
}

void write_output(char filename[]){

    //cout << "Output file : " << filename << endl;
    ofstream fout;
    fout.open(filename, ios::out);

    fout << "Address bits: " << Address_bits << endl;
    fout << "Block size: " << Block_size << endl;
    fout << "Cache sets: " << Cache_sets << endl;
    fout << "Associativity: " << Associativity << endl << endl;

    fout << "Offset bit count: " << log2(Block_size) << endl;
    fout << "Indexing bit count: " << log2(Cache_sets) << endl;
    fout << "Indexing bits: ";
    for(int i = 0 ; i < cache_index_size ; i++){
        fout << indexing_pattern[i] + offset_size << " ";
    } 
    fout << endl << endl;


    fout << ".benchmark " << benchmark << endl;
    for(int i = 0 ; i < testcase.size() ; i++){
        if(hit_or_miss[i] == 0){
            fout << testcase[i] << " miss" << endl;
        }
        else{
            fout << testcase[i] << " hit" << endl;
        }
    }

    fout << ".end" << endl << endl;
    fout << "Total cache miss count: " << best_miss_count << endl;

    fout.close();
}