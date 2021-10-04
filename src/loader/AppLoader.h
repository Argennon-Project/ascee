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
        argc::dispatcher_ptr_t dispatcher;
    };
    const std::filesystem::path libraryPath;
    std::unordered_map<argc::std_id_t, AppHandle> dispatchersMap;
    std::mutex mapMutex;

public:
    explicit AppLoader(std::string_view libraryPath);

    virtual ~AppLoader();

    void loadApp(argc::std_id_t);

    void unLoadApp(argc::std_id_t);

    void updateApp(argc::std_id_t);

    argc::dispatcher_ptr_t getDispatcher(argc::std_id_t appID);

    std::unordered_map<argc::std_id_t, argc::dispatcher_ptr_t>
    createAppTable(const std::vector<argc::std_id_t>& appList);

    static std::unique_ptr<AppLoader> global;
};

} // namespace ascee

#endif // ASCEE_APP_LOADER_H
