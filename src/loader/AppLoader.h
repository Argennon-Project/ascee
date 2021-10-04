#ifndef ASCEE_APP_LOADER_H
#define ASCEE_APP_LOADER_H

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <vector>
#include <mutex>

#include "../../include/argc/types.h"

namespace ascee {

class AppLoader {
private:
    struct AppHandle {
        void* handle;
        dispatcher_ptr_t dispatcher;
    };
    const std::filesystem::path libraryPath;
    std::unordered_map<std_id_t, AppHandle> dispatchersMap;
    std::mutex mapMutex;

public:
    explicit AppLoader(std::string_view libraryPath);

    virtual ~AppLoader();

    void loadApp(std_id_t);

    void unLoadApp(std_id_t);

    void updateApp(std_id_t);

    dispatcher_ptr_t getDispatcher(std_id_t appID);

    std::unordered_map<std_id_t, dispatcher_ptr_t> createAppTable(const std::vector<std_id_t>& appList);

    static std::unique_ptr<AppLoader> global;
};

} // namespace ascee

#endif // ASCEE_APP_LOADER_H
