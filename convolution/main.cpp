#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <numeric>
#include <chrono>
#ifdef THREADS
#  include <thread>
#  include <future>
#endif

#ifdef __EXCEPTIONS
#  define ERROR(MSG) throw std::runtime_error(MSG)
#else
#  define ERROR(MSG) { puts(MSG); exit(1); }
#endif

struct __attribute__((packed)) BMPFileHeader
{
    uint16_t file_type{0x4D42};
    uint32_t file_size{0};
    uint16_t reserverd1{0};
    uint16_t reserverd2{0};
    uint32_t offset_data{0};
};

struct __attribute__((packed)) BMPInfoHeader
{
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bit_count{0};
    uint32_t compression{0};
    uint32_t size_image{0};
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
};

struct __attribute__((packed)) BMPColourHeader
{
    uint32_t red_mask{0x00ff0000};
    uint32_t blue_mask{0x0000ff00};
    uint32_t green_mask{0x000000ff};
    uint32_t alpha_mask{0xff000000};
    uint32_t color_space_type{0x73524742};
    uint32_t unused[16]{0};
};

struct __attribute__((packed)) BMP
{
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    BMPColourHeader colour_header;
    std::vector<uint8_t> data;

    explicit BMP(const char* file_name)
    {
        read(file_name);
    }

    void read(const char* file_name)
    {
        std::ifstream file{file_name, std::ios::binary};
        if (file) {
            file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
            if (file_header.file_type != 0x4D42) {
                ERROR("Error! Unrecognized file format.");
            }

            file.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
            if (info_header.bit_count == 32) {
                if (info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColourHeader))) {
                    file.read(reinterpret_cast<char*>(&colour_header), sizeof(colour_header));
                    check_colour_header(colour_header);
                } else {
                    ERROR("Error! Unrecognized file format.");
                }
            }

            file.seekg(file_header.offset_data, file.beg);

            if (info_header.bit_count == 32) {
                info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColourHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColourHeader);
            } else {
                info_header.size = sizeof(BMPInfoHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            file_header.file_size = file_header.offset_data;

            if (info_header.height < 0) {
                ERROR(
                        "The program can treat only BMP images with the origin in the bottom left corner!");
            }

            data.resize(info_header.width * info_header.height * info_header.bit_count / 8);
            file_header.file_size += data.size();

            if (info_header.width % 4 == 0) {
                file.read(reinterpret_cast<char*>(data.data()), data.size());
            } else {
                row_stride = info_header.width * info_header.bit_count / 8;
                uint32_t new_stride = make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - row_stride);

                for (int y = 0; y < info_header.height; ++y) {
                    file.read(reinterpret_cast<char*>(data.data() + row_stride * y), row_stride);
                    file.read(reinterpret_cast<char*>(padding_row.data()), padding_row.size());
                }
            }
        } else {
            ERROR("Unable to open the input image file.");
        }
    }

    void write(const char* file_name)
    {
        std::ofstream file{file_name, std::ios::binary};
        if (file) {
            if (info_header.bit_count == 32) {
                write_headers_and_data(file);
            } else if (info_header.bit_count == 24) {
                if (info_header.width % 4 == 0) {
                    write_headers_and_data(file);
                } else {
                    uint32_t new_stride = make_stride_aligned(4);
                    std::vector<uint8_t> padding_row(new_stride - row_stride);
                    write_headers(file);
                    for (int y = 0; y < info_header.height; ++y) {
                        file.write((const char*)(data.data() + row_stride * y), row_stride);
                        file.write((const char*)padding_row.data(), padding_row.size());
                    }
                }
            } else {
                ERROR("The program can treat only 24 or 32 bits per pixel BMP files");
            }
        } else {
            ERROR("Unable to open the output image file.");
        }
    }

private:
    uint32_t row_stride{0};

    void check_colour_header(BMPColourHeader& colour_header)
    {
        BMPColourHeader expected_color_header;
        if (expected_color_header.red_mask != colour_header.red_mask ||
            expected_color_header.blue_mask != colour_header.blue_mask ||
            expected_color_header.green_mask != colour_header.green_mask ||
            expected_color_header.alpha_mask != colour_header.alpha_mask) {
            ERROR(
                    "Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
        }
        if (expected_color_header.color_space_type != colour_header.color_space_type) {
            ERROR("Unexpected color space type! The program expects sRGB values");
        }
    }

    uint32_t make_stride_aligned(uint32_t align_stride)
    {
        uint32_t new_stride = row_stride;
        while (new_stride % align_stride != 0) {
            new_stride++;
        }
        return new_stride;
    }

    void write_headers(std::ofstream& file)
    {
        file.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
        file.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));
        if (info_header.bit_count == 32) {
            file.write(reinterpret_cast<char*>(&colour_header), sizeof(colour_header));
        }
    }

    void write_headers_and_data(std::ofstream& file)
    {
        write_headers(file);
        file.write(reinterpret_cast<char*>(data.data()), data.size());
    }
};

std::vector<int> read_data(const char* file_name)
{
    std::ifstream file(file_name, std::ios::in);
    std::vector<int> data;
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line, ',')) {
            data.push_back(std::stoi(line));
        }
    }
    return data;
}

struct BMPPixelData
{
    explicit BMPPixelData(BMP& bmp)
            : data_{bmp.data}
            , width{bmp.info_header.width}
            , height{bmp.info_header.height},
              channels{bmp.info_header.bit_count / 8}
    {}

    [[nodiscard]] uint8_t r(int x, int y) const
    {
        return data_.at(channels * (y * width + x) + r_channel);
    }

    [[nodiscard]] uint8_t g(int x, int y) const
    {
        return data_.at(channels * (y * width + x) + g_channel);
    }

    [[nodiscard]] uint8_t b(int x, int y) const
    {
        return data_.at(channels * (y * width + x) + b_channel);
    }

    static void set(const BMPPixelData& data, std::vector<uint8_t>& buffer, int x, int y, uint8_t r, uint8_t g, uint8_t b)
    {
        buffer.at(data.channels * (y * data.width + x) + r_channel) = r;
        buffer.at(data.channels * (y * data.width + x) + g_channel) = g;
        buffer.at(data.channels * (y * data.width + x) + b_channel) = b;
    }

    void clear()
    {
        std::fill(data_.begin(), data_.end(), 0);
    }

    void update(const std::vector<uint8_t>& buffer, int y_begin, int x_begin, int h, int w)
    {
        int y_end = y_begin + h;
        int x_end = x_begin + w;

        for (int y = y_begin; y < y_end; ++y) {
            for (int x = x_begin; x < x_end; ++x) {
                int i = channels * (y * width + x);
                for (int n = 0; n < channels; ++n) {
                    data_.at(i+n) = buffer.at(i+n);
                }
            }
        }
    }

    int width;
    int height;
    const int channels;

private:
    static constexpr int b_channel = 0;
    static constexpr int g_channel = 1;
    static constexpr int r_channel = 2;

    std::vector<uint8_t>& data_;
};

template<int N>
struct Matrix
{
    explicit constexpr Matrix(const std::array<int, N * N>& data) : data_{data}
    {}

    [[nodiscard]] int at(int x, int y) const
    { return data_[x * N + y]; }

    [[nodiscard]] int sum() const
    {
        return std::accumulate(data_.begin(), data_.end(), 0, std::plus<int>());
    }

private:
    std::array<int, N * N> data_ = {};
};

template <int N>
std::vector<uint8_t> partial_convolution(const BMPPixelData& data, int y_begin, int x_begin, int height, int width, Matrix<N> matrix, double scale, bool grey_scale)
{
    int m_center = N / 2;
    int n_center = N / 2;

    std::vector<uint8_t> buffer(data.width * data.height * data.channels);

    int y_end = y_begin + height;
    int x_end = x_begin + width;

    for (int y = y_begin; y < y_end; ++y) {
        for (int x = x_begin; x < x_end; ++x) {
            double new_r = 0;
            double new_g = 0;
            double new_b = 0;

            for (int m = 0; m < N; ++m) {
                int mm = N - 1 - m;

                for (int n = 0; n < N; ++n) {
                    int nn = N - 1 - n;

                    int yy = y + (m_center - mm);
                    int xx = x + (n_center - nn);

                    if (yy >= 0 && yy < data.height && xx >= 0 && xx < data.width) {
                        auto r = data.r(xx, yy) * matrix.at(mm, nn);
                        auto g = data.g(xx, yy) * matrix.at(mm, nn);
                        auto b = data.b(xx, yy) * matrix.at(mm, nn);
                        new_r += r;
                        new_g += g;
                        new_b += b;
                    }
                }
            }

            new_r = new_r * scale;
            new_g = new_g * scale;
            new_b = new_b * scale;
            new_r = std::max(std::min(new_r, 255.), 0.);
            new_g = std::max(std::min(new_g, 255.), 0.);
            new_b = std::max(std::min(new_b, 255.), 0.);

            if (grey_scale) {
                double gray_scale = 0.299 * new_r + 0.587 * new_g + 0.114 * new_b;
                new_r = gray_scale;
                new_g = gray_scale;
                new_b = gray_scale;
            }

            BMPPixelData::set(data, buffer, x, y, (uint8_t)new_r, (uint8_t)new_g, (uint8_t)new_b);
        }
    }

#ifdef THREADS
    std::cout << "thread " << std::this_thread::get_id() << " finished" << std::endl;
#endif
    return buffer;
}

struct BlockParams {
    int y_begin;
    int x_begin;
};

template<int N>
void perform_convolution(BMPPixelData& data, Matrix<N> matrix, double scale, bool grey_scale)
{
#ifdef THREADS
    int n_blocks = 2;
    int h = data.height / n_blocks;
    int w = data.width / n_blocks;

    std::vector<std::pair<BlockParams, std::future<std::vector<uint8_t>>>> futures;

    for (int i = 0; i < n_blocks; ++i) {
        for (int j = 0; j < n_blocks; ++j) {
            int y = i * h;
            int x = j * w;
            std::future<std::vector<uint8_t>> ret = std::async(partial_convolution<N>, data, y, x, h, w, matrix, scale, grey_scale);
            futures.emplace_back(BlockParams{ y, x }, std::move(ret));
        }
    }

    for (auto& [params, future] : futures) {
        auto buffer = future.get();
        data.update(buffer, params.y_begin, params.x_begin, h, w);
    }
#else
    auto buffer = partial_convolution<N>(data, 0, 0, data.height, data.width, matrix, scale, grey_scale);
    data.update(buffer, 0, 0, data.height, data.width);
#endif
}

constexpr Matrix<3> ridge{
        {-1, -1, -1,
         -1, 8, -1,
         -1, -1, -1}
};

constexpr Matrix<3> sharpen{
        {0, -1, 0,
         -1, 5, -1,
         0, -1, 0}
};

constexpr Matrix<3> blur{
        {1, 1, 1,
         1, 1, 1,
         1, 1, 1}
};

constexpr Matrix<5> gaussian{
        {1, 4, 6, 4, 1,
         4, 16, 24, 16, 4,
         6, 24, 36, 24, 6,
         4, 16, 24, 16, 4,
         1, 4, 6, 4, 1}
};

int main()
{
    auto img = BMP("photo.bmp");
    auto data = BMPPixelData(img);
    std::cout << "file loaded" << std::endl;

    auto convolution = ridge;

    int sum = convolution.sum();
    if (sum == 0) {
        sum = 1;
    }
    double scale = 1. / sum;
    std::cout << "performing convolution" << std::endl;
    std::chrono::high_resolution_clock clock;
    auto start = clock.now();
    perform_convolution(data, convolution, scale, true);
    auto end = clock.now();
    std::cout << "done" << std::endl;
    std::chrono::duration<double> delta = end - start;
    std::cout << "took " << delta.count() << " seconds" << std::endl;

    img.write("output.bmp");
    std::cout << "file saved" << std::endl;

    return 0;
}
