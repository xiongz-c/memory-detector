#include <ncurses.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <iostream>
#include <pwd.h>
#include <algorithm>
#include <time.h>

#define DELAY 100000
#define DETAIL_LINE 6

using namespace std;

struct fdinfo {
    string fd;
    int pos;
    int flags;
};

struct process {
    string name = "";
    int pid = -1;
    int ppid = -1;
    int tgid = -1;
    string user = "";
    string state = "";
    unsigned long long VmPeak = 0;
    unsigned long long VmSize = 0;
    unsigned long long VmHWM = 0;
    unsigned long long VmRSS = 0;
    time_t start_time = 0;
    fdinfo fd;

    bool operator>(const process &r) const   //降序排序
    {
        return VmRSS > r.VmRSS;
    }
};

struct thread {
    string name = "";
    int pid = -1;
    int ppid = -1;
    int tgid = -1;
    string user = "";
    string state = "";
    unsigned long long VmPeak = 0;
    unsigned long long VmSize = 0;
    unsigned long long VmHWM = 0;
    unsigned long long VmRSS = 0;
    time_t start_time = 0;

    bool operator>(const thread &r) const   //降序排序
    {
        return VmRSS > r.VmRSS;
    }
};


unsigned long long tot_vm, used_vm, tot_mem, unused_mem;
int task_cnt, run_cnt, sleep_cnt, stop_cnt;
bool thread_flag = false, single_proc = false;
int single_pid = -1;
vector<process> proc_list;
vector<thread> thread_list;
vector<fdinfo> proc_fd;

string convert(int mode) {
    int a[20];
    int len = 0;
    while (mode != 0) {
        a[len] = mode % 10;
        mode /= 10;
        len++;
    }
    for (int i = len - 1; i >= 0; i--) {
        mode = mode * 8 + a[i];
    }
    string s = "";
    if (S_ISDIR(mode))
        s += 'd';
    else if (S_ISCHR(mode))
        s += 'c';
    else if (S_ISBLK(mode))
        s += 'b';
    else if (S_ISLNK(mode))
        s += 'l';
    else
        s += '-';

    s += (mode & S_IRUSR ? 'r' : '-');
    s += (mode & S_IWUSR ? 'w' : '-');
    s += (mode & S_IXUSR ? 'x' : '-');

    s += (mode & S_IRGRP ? 'r' : '-');
    s += (mode & S_IWGRP ? 'w' : '-');
    s += (mode & S_IWGRP ? 'x' : '-');

    s += (mode & S_IROTH ? 'r' : '-');
    s += (mode & S_IWOTH ? 'w' : '-');
    s += (mode & S_IXOTH ? 'x' : '-');

    return s;
}

int get_tot_task() {
    return task_cnt;
}

int get_runing_task() {
    return run_cnt;
}

int get_sleep_task() {
    return sleep_cnt;
}

int get_stop_task() {
    return stop_cnt;
}

unsigned long long get_mem_usage() {
    return tot_mem - unused_mem;
}

unsigned long long get_tot_mem() {
    return tot_mem;
}

unsigned long long get_tot_vm() {
    return tot_vm;
}

bool is_digit(const string &str) {
    return str.find_first_not_of("0123456789") == string::npos;
}

void get_mem_info() {
    ifstream infile;
    try {
        infile.open("/proc/meminfo");
        if (!infile.is_open()) {
            infile.close();
            return;
        }
        string s;
        while (infile >> s) {
            if (s == "VmallocTotal:")
                infile >> tot_vm;
            else if (s == "VmallocUsed:")
                infile >> used_vm;
            else if (s == "MemTotal:")
                infile >> tot_mem;
            else if (s == "MemFree:")
                infile >> unused_mem;
        }
        infile.close();
    } catch (exception &e) {
        infile.close();
    }
}

vector<string> get_thread_path_list(string task) {
    vector<string> threads;
    DIR *dir;
    try {
        if ((dir = opendir(task.c_str())) == NULL) {
            closedir(dir);
            return threads;
        }
        struct dirent *pDir;
        while ((pDir = readdir(dir)) != NULL) {
            if (is_digit(pDir->d_name) && pDir->d_type == 4) {
                threads.push_back(task + "/" + pDir->d_name);
            }
        }
        closedir(dir);
    } catch (exception &e) {
        closedir(dir);
    }
    return threads;
}

vector<string> get_process_path_list(string proc) {
    vector<string> processes;
    DIR *dir;
    try {
        if ((dir = opendir(proc.c_str())) == NULL) {
            closedir(dir);
            return processes;
        }
        struct dirent *pDir;
        while ((pDir = readdir(dir)) != NULL) {
            if (is_digit(pDir->d_name) && pDir->d_type == 4) {
                processes.push_back(proc + "/" + pDir->d_name);
            }
        }
        closedir(dir);
    } catch (exception e) {
        closedir(dir);
    }
    return processes;
}

string get_target_process_path(string proc, string pid) {
    string res;
    DIR *dir;
    try {
        if ((dir = opendir(proc.c_str())) == NULL) {
            closedir(dir);
            exit(0);
        }
        struct dirent *pDir;
        while ((pDir = readdir(dir)) != NULL) {
            if (is_digit(pDir->d_name) && pDir->d_type == 4 && pDir->d_name == pid) {
                closedir(dir);
                return proc + "/" + pDir->d_name;
            }
        }
        closedir(dir);
    } catch (exception &e) {
        closedir(dir);
    }
    return "";
}

void process_fd_info(string file, string _fd) {
    ifstream infile;
    try {
        infile.open(file, ios::in);
        if (!infile.is_open()) {
            infile.close();
            return;
        }
        string s;
        struct fdinfo fd;
        fd.fd = _fd;
        int index = 0;
        while (infile >> s) {
            if (s == "pos:") {
                infile >> fd.pos;
            } else if (s == "flags:" && index < 3) {
                infile >> fd.flags;
            }
            index++;
        }
        infile.close();
        proc_fd.push_back(fd);
    } catch (exception &e) {
        infile.close();
    }
}

void get_fd(string proc) {
    proc = proc + "/fdinfo";
    DIR *dir;
    try {
        if ((dir = opendir(proc.c_str())) == NULL) {
            closedir(dir);
            return;
        }
        struct dirent *pDir;
        while ((pDir = readdir(dir)) != NULL) {
            if (strcmp(pDir->d_name, ".") == 0 || strcmp(pDir->d_name, "..") == 0)
                continue;
            process_fd_info(proc + "/" + pDir->d_name, pDir->d_name);
            // cout << "\t" << fi.fd << " " << fi.flags << " " << convert(fi.flags) << " " << fi.pos << endl;
        }
        closedir(dir);
    } catch (exception &e) {
        closedir(dir);
    }
}

void cnt_jobs_status(string state) {
    if (state == "S" || state == "I") {
        ++sleep_cnt;
    } else if (state == "R") {
        ++run_cnt;
    } else if (state == "T") {
        ++stop_cnt;
    }
}

void get_process_info_from_path(vector<string> files) {
    for (auto file : files) {
        ifstream infile;
        try {
            infile.open(file + "/status", ios::in);
            if (!infile.is_open()) {
                infile.close();
                return;
            }
            string s;
            struct process cur;
            while (infile >> s) {
                if (s == "Name:") {
                    infile >> cur.name;
                    if (cur.name.length() > 5)
                        cur.name = cur.name.substr(0, 6) + "+";
                } else if (s == "State:") {
                    infile >> cur.state;
                } else if (s == "Pid:") {
                    infile >> cur.pid;
                } else if (s == "PPid:") {
                    infile >> cur.ppid;
                } else if (s == "VmPeak:") {
                    infile >> cur.VmPeak;
                } else if (s == "VmSize:") {
                    infile >> cur.VmSize;
                } else if (s == "Tgid:") {
                    infile >> cur.tgid;
                } else if (s == "VmHWM:") {
                    infile >> cur.VmHWM;
                } else if (s == "VmRSS:") {
                    infile >> cur.VmRSS;
                } else if (s == "Uid:") {
                    uid_t uid;
                    struct passwd *user;
                    infile >> uid;
                    user = getpwuid(uid);
                    cur.user = user->pw_name;
                    if (cur.user.length() > 5)
                        cur.user = cur.user.substr(0, 6) + "+";
                }
            }
            infile.close();
            infile.open(file + "/stat", ios::in);
            if (!infile.is_open()) {
                infile.close();
                return;
            }
            int index = 1;
            unsigned long long start_time = 0;
            while (true) {
                if (index == 22) {
                    infile >> start_time;
                    break;
                }
                infile >> s;
                index++;
            }
            infile.close();
            struct sysinfo info;
            if (sysinfo(&info)) {
                cout << "Error" << endl;
                exit(0);
            }
            time_t cur_time = 0;
            time_t boot_time = 0;
            time(&cur_time);
            if (cur_time > info.uptime)
                boot_time = cur_time - info.uptime;
            else
                boot_time = info.uptime - cur_time;
            cur.start_time = boot_time + start_time / 100;
            cnt_jobs_status(cur.state);
            proc_list.push_back(cur);
        } catch (exception &e) {
            infile.close();
        }
    }
    sort(proc_list.begin(), proc_list.end(), greater<process>());
}

void get_thread_info_from_path(vector<string> files) {
    for (auto file : files) {
        ifstream infile;
        try {
            infile.open(file + "/status", ios::in);
            if (!infile.is_open()) {
                infile.close();
                return;
            }
            string s;
            struct thread cur;
            while (infile >> s) {
                if (s == "Name:") {
                    infile >> cur.name;
                    if (cur.name.length() > 5)
                        cur.name = cur.name.substr(0, 6) + "+";
                } else if (s == "State:") {
                    infile >> cur.state;
                } else if (s == "Pid:") {
                    infile >> cur.pid;
                } else if (s == "PPid:") {
                    infile >> cur.ppid;
                } else if (s == "VmPeak:") {
                    infile >> cur.VmPeak;
                } else if (s == "VmSize:") {
                    infile >> cur.VmSize;
                } else if (s == "Tgid:") {
                    infile >> cur.tgid;
                } else if (s == "VmHWM:") {
                    infile >> cur.VmHWM;
                } else if (s == "VmRSS:") {
                    infile >> cur.VmRSS;
                } else if (s == "Uid:") {
                    uid_t uid;
                    struct passwd *user;
                    infile >> uid;
                    user = getpwuid(uid);
                    cur.user = user->pw_name;
                    if (cur.user.length() > 5)
                        cur.user = cur.user.substr(0, 6) + "+";
                }
            }
            infile.close();
            infile.open(file + "/stat", ios::in);
            if (!infile.is_open()) {
                infile.close();
                return;
            }
            int index = 1;
            unsigned long long start_time = 0;
            while (true) {
                if (index == 22) {
                    infile >> start_time;
                    break;
                }
                infile >> s;
                index++;
            }
            infile.close();
            struct sysinfo info;
            if (sysinfo(&info)) {
                cout << "Error" << endl;
                exit(0);
            }
            time_t cur_time = 0;
            time_t boot_time = 0;
            time(&cur_time);
            if (cur_time > info.uptime)
                boot_time = cur_time - info.uptime;
            else
                boot_time = info.uptime - cur_time;
            cur.start_time = boot_time + start_time / 100;
            cnt_jobs_status(cur.state);
            thread_list.push_back(cur);
        } catch (exception &e) {
            infile.close();
        }
    }
}

void init_console() {
    initscr(); /* 初始化屏幕 */

    noecho(); /* 屏幕上不返回任何按键 */

    curs_set(FALSE); /* 不显示光标 */

    refresh(); /* 更新显示器 */

    sleep(1);
}

void resize_console() {
    int now_x, now_y;
    getmaxyx(stdscr, now_y, now_x); /* 获取屏幕尺寸 */
    while (now_y < 15) {
        clear();
        if (now_x > 50) {
            mvprintw(0, 0, "Console size: (%d,%d) too small, please enlarge console!", now_x, now_y);
        } else {
            mvprintw(0, 0, "Enlarge console!");
        }
        refresh();
        usleep(DELAY);
        getmaxyx(stdscr, now_y, now_x);
    }
}

void print_logo(int now_x, int target_y, string logo) {
    int info_beg = now_x / 2 - logo.length() / 2;
    int info_end = now_x / 2 + logo.length() / 2;
    for (int i = 0; i < info_beg; i++) {
        mvprintw(target_y, i, "=");
    }
    mvprintw(target_y, now_x / 2 - logo.length() / 2, logo.c_str());
    for (int i = info_end; i < now_x; i++) {
        mvprintw(target_y, i, "=");
    }
}

void print_base_info(int now_x, int now_y) {
    mvprintw(0, 0, "Welcome to SusTop");
    if (single_proc) {
        mvprintw(0, 20, "< Single Process Monitor >");
    } else if (thread_flag) {
        mvprintw(0, 20, "< Thread Monitor >");
    } else {
        mvprintw(0, 20, "< Process Monitor >");
    }
    mvprintw(1, 0, "Task Info: %d total, %d running, %d sleeping, %d stopped", get_tot_task(), get_runing_task(),
             get_sleep_task(), get_stop_task());
    string device_info = "=Device Info=";
    print_logo(now_x, 2, device_info);
    mvprintw(4, 0, "MEM usage: %lld kB used,  %lld kB total", get_mem_usage(), get_tot_mem());

}

void print_proc_info(int max_y, int beg_line) {
    mvprintw(beg_line, 0, "%-8s\t%-8s\t%-8s\t%-8s\t%s\t%-8s\t%-8s\t%-8s\t%-8s\t%-10s\t",
             "COMMAND", "USER", "PID", "PPID", "STATE", "VM", "RSS", "%MEM", "%MEM(peak)", "BEGIN TIME");
    ++beg_line;
    for (auto proc : proc_list) {
        mvprintw(beg_line, 0, "%-8s\t%-8s\t%-8d\t%-8d\t%s\t%-8lld\t%-8lld\t%-8.2f\t%-8.2f\t%-10s\t",
                 proc.name.c_str(), proc.user.c_str(), proc.pid, proc.ppid, proc.state.c_str(), proc.VmSize, proc.VmRSS,
                 (double) proc.VmRSS / get_tot_mem() * 100,
                 (double) proc.VmHWM / get_tot_mem() * 100, ctime(&proc.start_time));
        ++beg_line;
        if (beg_line >= max_y - 1) {
            break;
        }
    }
}

int print_thread_info(int max_y, int beg_line) {
    mvprintw(beg_line, 0, "%-8s\t%-8s\t%-8s\t%-8s\t%s\t%-8s\t%-8s\t%-8s\t%-8s\t%-10s\t",
             "COMMAND", "USER", "PID", "PPID", "STATE", "VM", "RSS", "%MEM", "%MEM(peak)", "BEGIN TIME");
    ++beg_line;
    for (auto proc : thread_list) {
        mvprintw(beg_line, 0, "%-8s\t%-8s\t%-8d\t%-8d\t%s\t%-8lld\t%-8lld\t%-8.2f\t%-8.2f\t%-10s\t",
                 proc.name.c_str(), proc.user.c_str(), proc.pid, proc.ppid, proc.state.c_str(), proc.VmSize, proc.VmRSS,
                 (double) proc.VmRSS / get_tot_mem() * 100,
                 (double) proc.VmHWM / get_tot_mem() * 100, ctime(&proc.start_time));
        ++beg_line;
        if (beg_line >= max_y - 1) {
            break;
        }
    }
    return beg_line;
}

int print_fd_info(int max_x, int max_y, int beg_line) {
    string fd_str = "=Fd Info=";
    print_logo(max_x, beg_line, fd_str);
    ++beg_line;
    mvprintw(beg_line, 0, "%-8s\t%-8s\t", "fd", "flags");
    ++beg_line;
    for (auto fi : proc_fd) {
        mvprintw(beg_line, 0, "%-8s\t%d\t", fi.fd.c_str(), fi.flags);
        ++beg_line;
        if (beg_line >= max_y - 1) {
            break;
        }
    }
    return beg_line;
}

void print_single_proc_info(int max_x, int max_y, int beg_line) {
    int nxt_line = print_thread_info(max_y, beg_line + 1);
    print_fd_info(max_x, max_y, nxt_line + 1);
}

void print_detail_info(int max_x, int max_y) {
    int beg_line = DETAIL_LINE + 1;
    if (single_proc) {
        print_single_proc_info(max_x, max_y, beg_line);
    } else if (thread_flag) {
        print_thread_info(max_y, beg_line);
    } else {
        print_proc_info(max_y, beg_line);
    }
}

void init_data() {
    get_mem_info();
    task_cnt = sleep_cnt = run_cnt = stop_cnt = 0;
    if (single_proc) {
        thread_list.clear();
        proc_fd.clear();
        string pwt = get_target_process_path("/proc", to_string(single_pid));
        vector<string> twt = get_thread_path_list(pwt + "/task");
        get_thread_info_from_path(twt);
        task_cnt = thread_list.size();
        get_fd(pwt);
        sort(thread_list.begin(), thread_list.end(), greater<thread>());
    } else if (thread_flag) {
        thread_list.clear();
        vector<string> pwt = get_process_path_list("/proc");
        for (auto p : pwt) {
            vector<string> twt = get_thread_path_list(p + "/task");
            get_thread_info_from_path(twt);
        }
        task_cnt = thread_list.size();
        sort(thread_list.begin(), thread_list.end(), greater<thread>());
    } else {
        proc_list.clear();
        vector<string> pwt = get_process_path_list("/proc");
        get_process_info_from_path(pwt);
        task_cnt = proc_list.size();
    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        string tmp = argv[i];
        if (tmp == "-t")
            thread_flag = true;
        if (tmp == "-p" && i + 1 < argc && is_digit(argv[i + 1])) {
            single_pid = atoi(argv[i + 1]);
            single_proc = true;
        }
    }
    int max_x = 0, max_y = 0;
    init_console();
    while (1) {
        resize_console();
        init_data();
        getmaxyx(stdscr, max_y, max_x); /* 获取屏幕尺寸 */
        clear();                        /* 清屏 */
        print_base_info(max_x, max_y);
        print_detail_info(max_x, max_y);
        refresh();
        usleep(DELAY);
    }
    endwin(); /* 恢复终端 */
    return 0;
}