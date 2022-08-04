#include "FileWriter.hpp"
#include <unistd.h>
#define WRITE_SIZE 1024
#define NON_FD -1

FileWriter::FileWriter(const RequestMatchingResult &match_result, const byte_string &content_to_write)
    : file_path_(HTTP::restrfy(match_result.path_local))
    , content_to_write_(content_to_write)
    , originated_(false)
    , fd_(NON_FD) {}

FileWriter::~FileWriter() {
    close_if_needed();
}

void FileWriter::notify(IObserver &observer, IObserver::observation_category cat, t_time_epoch_ms epoch) {
    (void)observer;
    (void)cat;
    (void)epoch;
    assert(false);
}
void FileWriter::write_to_file() {
    // TODO: C++ way に書き直す
    if (originated_) {
        return;
    }
    int fd = open(file_path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) {
        switch (errno) {
            case EACCES:
                throw http_error("permission denied", HTTP::STATUS_FORBIDDEN);
            default:
                QVOUT(strerror(errno));
                throw http_error("can't open", HTTP::STATUS_FORBIDDEN);
        }
    }
    fd_                               = fd;
    byte_string::size_type write_head = 0;
    ssize_t written_size              = 0;
    for (; write_head < content_to_write_.size();) {
        byte_string::size_type write_max
            = std::min((byte_string::size_type)WRITE_SIZE, content_to_write_.size() - write_head);

        written_size = write(fd, &content_to_write_[write_head], write_max);
        if (written_size < 0) {
            throw http_error("read error", HTTP::STATUS_FORBIDDEN);
        }
        write_head += written_size;
        if (written_size == 0) {
            DXOUT("short?");
            break;
        }
    }
    HTTP::byte_string written_size_str = ParserHelper::utos(write_head, 10);
    response_data.inject(&written_size_str.front(), written_size_str.size(), true);
    originated_ = true;
    close_if_needed();
}

void FileWriter::close_if_needed() {
    if (fd_ < 0) {
        return;
    }
    close(fd_);
    fd_ = NON_FD;
}

void FileWriter::inject_socketlike(ISocketLike *socket_like) {
    (void)socket_like;
}

bool FileWriter::is_originatable() const {
    return !originated_;
}

bool FileWriter::is_origination_started() const {
    return originated_;
}

bool FileWriter::is_reroutable() const {
    return false;
}

bool FileWriter::is_responsive() const {
    return originated_;
}

void FileWriter::start_origination(IObserver &observer) {
    (void)observer;
    write_to_file();
}

void FileWriter::leave() {
    delete this;
}

ResponseHTTP *FileWriter::respond(const RequestHTTP &request) {
    response_data.determine_sending_mode();
    ResponseHTTP *res = new ResponseHTTP(request.get_http_version(), HTTP::STATUS_OK, NULL, &response_data);
    res->start();
    return res;
}
