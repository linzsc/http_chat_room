#include "global.h"
#include <set>
#include "EpollReactor.h"
EpollReactor server;
std::set<int>online_members;
std::mutex online_mutex;
std::unordered_map<int, ClientContext> client_contexts;