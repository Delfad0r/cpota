#include <algorithm>
#include <array>
#include <assert.h>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <queue>
#include <random>
#include <set>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>


typedef long long ll;


class PotaError : public std::exception {
    
    std::string error;
    
public:
    
    PotaError(const std::string & s = "") : error(s) {
        
    }
    
    const char* what() const noexcept {
        return error.c_str();
    }
    
};


class Code {
    
    std::unordered_map<ll, std::unordered_map<ll, std::string>> code;
    std::unordered_map<ll, std::set<ll>> rows, cols;
    
public:
    
    void set(const ll& x, const ll& y, const std::string& v) {
        if(v == " ") {
            if(code.count(x) && code[x].count(y)) {
                code[x].erase(y);
                rows[y].erase(x);
                cols[x].erase(y);
            }
        } else {
            code[x][y] = v;
            rows[y].insert(x);
            cols[x].insert(y);
        }
    }
    
    std::string get(const ll& x, const ll& y) {
        if(code.count(x) && code[x].count(y)) {
            return code[x][y];
        } else {
            return " ";
        }
    }
    
    ll get_maxw(const ll& y) {
        if(rows.count(y) && rows[y].size()) {
            return std::max(0ll, *rows[y].rbegin());
        } else {
            return 0;
        }            
    }
    
    ll get_maxh(const ll& x) {
        if(cols.count(x) && cols[x].size()) {
            return std::max(0ll, *cols[x].rbegin());
        } else {
            return 0;
        }
    }
    
    Code() {
        
    }
    
    Code(std::vector<std::string> lines) {
        if(lines.size() && lines.front().length() >= 2 &&
            lines.front()[0] == '#' && lines.front()[1] == '!') {
                lines.erase(lines.begin());
        }
        for(ll y = 0; y < ll(lines.size()); ++y) {
            for(ll x = 0; x < ll(lines[y].size()); ++x) {
                set(x, y, std::string(1, lines[y][x]));
            }
        }
    }
    
};


class Either {

    ll num;
    std::string str;
    bool num_ok, str_ok;
    
public:
    
    Either(const ll& num_) {
        *this = num_;
    }
    
    Either(const std::string& str_) {
        *this = str_;
    }
    
    ll& n() {
        if(!num_ok) {
            num_ok = true;
            num = std::stoll(str);
        }
        return num;
    }
    
    std::string& s() {
        if(!str_ok) {
            str_ok = true;
            str = std::to_string(num);
        }
        return str;
    }
    
    Either& operator = (const ll& num_) {
        num_ok = true;
        str_ok = false;
        num = num_;
        return *this;
    }
    
    Either& operator = (const std::string& str_) {
        str_ok = true;
        num_ok = false;
        str = str_;
        return *this;
    }
};


std::mt19937 rng;
Code code;


class Pointer {
    
    static unsigned free_id;
    static std::array<std::function<void(Pointer&)>, 256> functions;
    
    unsigned id;
    ll x, y, dirx, diry;
    bool is_alive, must_skip;
    char string_mode;
    std::vector<std::deque<Either>> stacks;
    std::deque<char> instructions;
    std::queue<Either> messages;

public:

    static std::unordered_map<unsigned, Pointer> ptrs;
    
    static void init() {
        free_id = 0;
        for(unsigned i = 0; i < functions.size(); ++i) {
            functions[i] = [&](Pointer&) {
                throw PotaError("Invalid instruction" + std::string(1, char(i)));
            };
        }
        // NOP
        functions[' '] = [&](Pointer&) { };
        // Arrows
        functions['<'] = [&](Pointer& ptr) {
            ptr.dirx = -1;
            ptr.diry = 0;
        };
        functions['>'] = [&](Pointer& ptr) {
            ptr.dirx = 1;
            ptr.diry = 0;
        };
        functions['^'] = [&](Pointer& ptr) {
            ptr.dirx = 0;
            ptr.diry = -1;
        };
        functions['v'] = [&](Pointer& ptr) {
            ptr.dirx = 0;
            ptr.diry = 1;
        };
        // Mirrors
        functions['/'] = [&](Pointer& ptr) {
            ll dirx_ = -ptr.diry;
            ll diry_ = -ptr.dirx;
            ptr.dirx = dirx_;
            ptr.diry = diry_;
        };
        functions['\\'] = [&](Pointer& ptr) {
            std::swap(ptr.dirx, ptr.diry);
        };
        functions['|'] = [&](Pointer& ptr) {
            ptr.dirx = -ptr.dirx;
        };
        functions['_'] = [&](Pointer& ptr) {
            ptr.diry = -ptr.diry;
        };
        functions['x'] = [&](Pointer& ptr) {
            functions["/\\|_"[std::uniform_int_distribution<>(0, 3)(rng)]](ptr);
        };
        // Skip
        functions['!'] = [&](Pointer& ptr) {
            ptr.must_skip = true;
        };
        // CondSkip
        functions['?'] = [&](Pointer& ptr) {
            ptr.must_skip = (ptr.pop().s() != "0");
        };
        // Where
        functions['w'] = [&](Pointer& ptr) {
            ptr.push(ptr.x);
            ptr.push(ptr.y);
        };
        // Jump
        functions['j'] = [&](Pointer& ptr) {
            ptr.y = ptr.pop().n();
            ptr.x = ptr.pop().n();
            if(ptr.x < 0 || ptr.y < 0) {
                throw PotaError("Jump to negative position");
            }
        };
        // Digits
        for(char c = '0'; c <= '9'; ++c) {
            functions[c] = [&, c](Pointer& ptr) {
                ptr.push(std::string(1, c));
            };
        }
        // Arith
        functions['+'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(x + y);
        };
        functions['-'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(x - y);
        };
        functions['*'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(x * y);
        };
        functions['%'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(x / y);
            ptr.push(x % y);
        };
        // Concat
        functions['.'] = [&](Pointer& ptr) {
            ptr.push(ptr.pop().s() + ptr.pop().s());
        };
        // Cmp
        functions['='] = [&](Pointer& ptr) {
            std::string y = ptr.pop().s();
            std::string x = ptr.pop().s();
            ptr.push(x == y);
        };
        functions['('] = [&](Pointer& ptr) {
            std::string y = ptr.pop().s();
            std::string x = ptr.pop().s();
            ptr.push(x < y);
        };
        functions[')'] = [&](Pointer& ptr) {
            std::string y = ptr.pop().s();
            std::string x = ptr.pop().s();
            ptr.push(x > y);
        };
        functions['['] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(x < y);
        };
        functions[']'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(x > y);
        };
        // Duplicate
        functions[','] = [&](Pointer& ptr) {
            Either x = ptr.pop();
            ptr.push(x);
            ptr.push(x);
        };
        // Pop
        functions['~'] = [&](Pointer& ptr) {
            ptr.pop();
        };
        // Swap
        functions['$'] = [&](Pointer& ptr) {
            Either y = ptr.pop();
            Either x = ptr.pop();
            ptr.push(y);
            ptr.push(x);
        };
        // Flatten
        functions[':'] = [&](Pointer& ptr) {
            std::string acc;
            for(Either & x : ptr.stacks.back()) {
                acc += x.s();
            }
            ptr.stacks.back().clear();
            ptr.push(acc);
        };
        // Rotate
        functions['{'] = [&](Pointer& ptr) {
            if(ptr.stacks.back().size()) {
                ptr.push(ptr.stacks.back().front());
                ptr.stacks.back().pop_front();
            }
        };
        functions['}'] = [&](Pointer& ptr) {
            if(ptr.stacks.back().size()) {
                ptr.stacks.back().push_front(ptr.pop());
            }
        };
        // Reverse
        functions['r'] = [&](Pointer& ptr) {
            std::reverse(ptr.stacks.back().begin(), ptr.stacks.back().end());
        };
        // Explode
        functions['e'] = [&](Pointer& ptr) {
            std::string x = ptr.pop().s();
            ptr.stacks.emplace_back();
            for(char c : x) {
                ptr.stacks.back().push_back(std::string(1, c));
            }
        };
        // New
        functions['n'] = [&](Pointer& ptr) {
            ll cnt = ptr.pop().n();
            std::deque<Either> new_stack;
            while(cnt--) {
                new_stack.push_front(ptr.pop());
            }
            ptr.stacks.emplace_back(std::move(new_stack));
        };
        // Merge
        functions['m'] = [&](Pointer& ptr) {
            std::deque<Either> old_stack = std::move(ptr.stacks.back());
            ptr.stacks.pop_back();
            if(ptr.stacks.size()) {
                ptr.stacks.back().insert(ptr.stacks.back().end(), old_stack.begin(), old_stack.end());
            } else {
                ptr.stacks.emplace_back();
            }
        };
        // SDuplicate
        functions['d'] = [&](Pointer& ptr) {
            ptr.stacks.push_back(ptr.stacks.back());
        };
        // Length
        functions['l'] = [&](Pointer& ptr) {
            ptr.push(ptr.stacks.back().size());
        };
        // Exec
        functions['`'] = [&](Pointer& ptr) {
            std::string x = ptr.pop().s();
            ptr.instructions.insert(ptr.instructions.begin(), x.begin(), x.end());
        };
        // Get
        functions['g'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            ptr.push(code.get(x, y));
        };
        // Put
        functions['p'] = [&](Pointer& ptr) {
            ll y = ptr.pop().n();
            ll x = ptr.pop().n();
            std::string v = ptr.pop().s();
            code.set(x, y, v);
        };
        // Spawn
        functions['&'] = [&](Pointer& ptr) {
            ll cnt = ptr.pop().n();
            Pointer new_ptr(ptr.stacks.back().begin(), ptr.stacks.back().begin(),
                ptr.dirx, ptr.diry, ptr.x + ptr.dirx, ptr.y + ptr.diry);
            while(cnt--) {
                new_ptr.stacks.back().push_front(ptr.pop());
            }
            ptrs.emplace(new_ptr.id, std::move(new_ptr));
        };
        // Wait
        functions['#'] = [&](Pointer& ptr) {
            if(ptr.messages.empty()) {
                ptr.instructions.push_front('#');
            } else {
                ptr.push(ptr.messages.front());
                ptr.messages.pop();
            }
        };
        // Send
        functions['@'] = [&](Pointer& ptr) {
            ll at = ptr.pop().n();
            try {
                ptrs.at(at).messages.emplace(ptr.pop());
            } catch(std::out_of_range) {
                throw PotaError("Pointer " + std::to_string(at) + " does not exist");
            }
        };
        // Id
        functions['y'] = [&](Pointer& ptr) {
            ptr.push(ptr.id);
        };
        // Chr
        functions['c'] = [&](Pointer& ptr) {
            ptr.push(std::string(1, char(ptr.pop().n())));
        };
        // Ord
        functions['a'] = [&](Pointer& ptr) {
            std::string x = ptr.pop().s();
            if(x.length() != 1) {
                throw PotaError("Ord failed (expected a single char)");
            } else {
                ptr.push(ll(x[0]));
            }
        };
        // In
        functions['i'] = [&](Pointer& ptr) {
            int c = getchar();
            if(c == EOF) {
                ptr.push(std::string());
            } else {
                ptr.push(std::string(1, char(c)));
            }
        };
        // Out
        functions['o'] = [&](Pointer& ptr) {
            printf("%s", ptr.pop().s().c_str());
            fflush(stdout);
        };
        // Die
        functions[';'] = [&](Pointer& ptr) {
            ptr.is_alive = false;
        };
    }
    
    
    template<typename I>
    Pointer(I begin_, I end_, ll dirx_ = 1, ll diry_ = 0, ll x_ = 0, ll y_ = 0) {
        id = free_id++;
        dirx = dirx_;
        diry = diry_;
        x = x_;
        y = y_;
        string_mode = '\0';
        is_alive = true;
        must_skip = false;
        stacks.emplace_back(begin_, end_);
        std::string str(code.get(x, y));
        instructions.insert(instructions.begin(), str.begin(), str.end());
    }
    
    void push(const Either& v) {
        stacks.back().push_back(v);
    }
    
    Either pop() {
        if(stacks.back().empty()) {
            throw PotaError("Tried to pop from empty stack");
        }
        Either v = std::move(stacks.back().back());
        stacks.back().pop_back();
        return v;
    }
    
    void exec_instruction(const char instr) {
        // Skip
        if(must_skip) {
            must_skip = false;
            return;
        }
        // String mode
        if((instr == '\'' || instr == '"') && !string_mode) {
            string_mode = instr;
            push(std::string());
        } else if(instr == string_mode) {
            string_mode = '\0';
        } else if(string_mode) {
            stacks.back().back().s() += instr;
        }else {
            functions[instr](*this);
        }
    }
    
    bool move() {
        if(instructions.size()) {
            char instr = instructions.front();
            instructions.pop_front();
            exec_instruction(instr);
        } else {
            x += dirx;
            y += diry;
            if(diry < 0 && y < 0) {
                must_skip = false;
                y = code.get_maxh(x);
            } else if(diry > 0 && y > code.get_maxh(x)) {
                must_skip = false;
                y = 0;
            } else if(dirx < 0 && x < 0) {
                must_skip = false;
                x = code.get_maxw(y);
            } else if(dirx > 0 && x > code.get_maxw(y)) {
                must_skip = false;
                x = 0;
            }
            if(must_skip) {
                must_skip = false;
            } else {
                std::string str(code.get(x, y));
                instructions.insert(instructions.begin(), str.begin(), str.end());
            }
        }
        return is_alive && (instructions.empty() || instructions.front() != '#');
    }
    
    bool alive() const {
        return is_alive;
    }
    
};

std::unordered_map<unsigned, Pointer> Pointer::ptrs;
unsigned Pointer::free_id;
std::array<std::function<void(Pointer&)>, 256> Pointer::functions;


int main(int argc, char* argv[]) {
    auto print_usage = [&]() {
        fprintf(stderr, "Usage: %s <script> [-s <vals>]\n", argv[0]);
    };
    
    std::vector<std::string> first_stack;
    if(argc < 2) {
        print_usage();
        exit(1);
    }
    std::ifstream src_code_file(argv[1]);
    if(!src_code_file) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        exit(1);
    }
    std::vector<std::string> lines(1);
    while(std::getline(src_code_file, lines.back())) {
        lines.push_back("");
    }
    src_code_file.close();
    lines.pop_back();
    code = Code(lines);
    if(argc > 2) {
        if(std::string(argv[2]) != "-s") {
            print_usage();
            exit(1);
        }
        for(int i = 3; i < argc; ++i) {
            first_stack.push_back(argv[i]);
        }
    }
    
    std::random_device rd;
    rng = std::mt19937(rd());
    
    termios old_settings, new_settings;
    tcgetattr(STDIN_FILENO, &old_settings);
    new_settings = old_settings;
    //cfmakeraw(&new_settings);
    new_settings.c_lflag &= ~(ECHO | ECHONL | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    
    Pointer::init();
    Pointer::ptrs.emplace(0, Pointer(first_stack.begin(), first_stack.end()));
    
    try {
        while(Pointer::ptrs.size()) {
            std::vector<unsigned> ids;
            for(auto& p : Pointer::ptrs) {
                ids.push_back(p.first);
            }
            for(unsigned id : ids) {
                Pointer& ptr = Pointer::ptrs.at(id);
                while(ptr.move()) { }
                if(!ptr.alive()) {
                    Pointer::ptrs.erase(id);
                }
            }
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
        printf("\n");
    } catch(PotaError& e) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
        fprintf(stderr, "\nPota! %s\n", e.what());
    }
}
