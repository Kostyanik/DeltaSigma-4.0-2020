#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <cmath>
#include <tuple>

// хэдеры для работы с .npy
#include <array> // в npy.hpp забыли заинклюдить
#include <npy.hpp>

// хэдеры для работы с .csv
#include <csv2/reader.hpp>
#include <csv2/writer.hpp>


/* вывод части вектора на экран */
template <typename DTYPE>
void printVector(const  std::vector<DTYPE> &data)
{
    std::cout << "[";

    const size_t nVisible = 5;
    const size_t size = data.size();
    size_t i = 0;

    for ( ; i < size && i < nVisible ; ++i )
    {
        std::cout << data[i];
        if ( i != size - 1 )
        {
            std::cout << ", ";
        }
    }

    if ( nVisible * 2 < size )
    {
        std::cout << "... , ";
    }

    i = ( (i + nVisible >= size) ? i : size - nVisible);
    for ( ; i < size ; ++i )
    {
        std::cout << data[i];
        if ( i != size - 1 )
        {
            std::cout << ", ";
        }
    }
    std::cout << "]";
}


/* пример загрузки npy */
template <typename DTYPE>
std::tuple<std::vector<DTYPE>, std::vector<unsigned long>, bool> loadNpy(const std::string &fileName)
{
    // размерность массива
    std::vector<unsigned long> shape;
    // ориентация массива - Си или Фортран
    bool isFortranOrder;
    // данные
    std::vector<DTYPE> data;

    // считывание данных с диска
    npy::LoadArrayFromNumpy(fileName, shape, isFortranOrder, data);

    // вывод на экран
    std::cout << "npy" << std::endl;

    // вывод типа ориентации на экран
    std::cout << "order: " << (isFortranOrder ? "F" : "C") << std::endl;

    // вывод размерности на экран
    std::cout << "shape: [";
    for ( size_t i = 0, size = shape.size() ; i < size ; ++i )
    {
        std::cout << shape[i];
        if ( i != size - 1 )
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    // вывод данных на экран
    std::cout << "data: ";
    printVector(data);
    std::cout << std::endl;

    return { data, shape, isFortranOrder };
}


/* пример загрузки csv */
template <typename DTYPE>
std::vector< std::vector<DTYPE> > loadCsv(const std::string &fileName)
{
    csv2::Reader<
            csv2::delimiter<','>,
            csv2::quote_character<'"'>,
            csv2::first_row_is_header<true>,
            csv2::trim_policy::trim_whitespace
            > csvReader;

    std::vector< std::vector<DTYPE> > dataColumns;

    if ( csvReader.mmap(fileName) )
    {
        const size_t rowCount = csvReader.rows();
        const size_t columnCount = csvReader.cols();
//        const auto header = csv.header();

        // таблица результатов, записанная по колонкам
        dataColumns.resize(columnCount);

        // считывание из файла и запись в таблицу результатов
        size_t iRow = 1; // хедер пропускаем
        auto row = csvReader.begin();
        for ( ; iRow < rowCount ; ++row, ++iRow)
        {
            size_t iColumn = 0;
            auto cell = (*row).begin();
            /* NOTE если делать через range-for или полностью через итераторы,
             * то не считывается последний столбец, если в нем есть пустые ячейки */
            for ( ; iColumn < columnCount ; ++cell, ++iColumn)
            {
                std::string strValue;
                (*cell).read_value(strValue);

                DTYPE value;
                try
                {
                    value = static_cast<DTYPE>(
                                std::is_same<DTYPE, float>::value ? std::stof(strValue)
                                                                  : std::stod(strValue)
                                                                    );
                }
                catch (...)
                {
                    value = std::numeric_limits<DTYPE>::quiet_NaN();
                }
                dataColumns[iColumn].push_back(value);
            }
        }

        std::cout << "csv data (without header): [" << rowCount - 1 << ", " << columnCount << "]" << std::endl;
        std::cout << "columns from csv:" << std::endl;
        for ( const auto &oneColumn : dataColumns )
        {
            printVector(oneColumn);
            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << "cannot open csv file" << std::endl;
    }

    return dataColumns;
}


/* пример сохранения csv */
template <typename IterX, typename IterY>
void writeCsv(IterX xBegin, IterX xEnd, IterY yBegin, IterY yEnd, const std::string &fileName)
{
    std::ofstream stream(fileName);
    csv2::Writer< csv2::delimiter<','> > csvWriter(stream);

    // запись хэдера
    std::vector<std::string> strRow = {"x", "y"};
    csvWriter.write_row(strRow);

    // запись построчно
    for ( ; xBegin != xEnd && yBegin != yEnd ; ++xBegin, ++yBegin)
    {
        strRow[0] = std::to_string(*xBegin);
        strRow[1] = ( std::isnan(*yBegin) ? "" : std::to_string(*yBegin) );
        csvWriter.write_row(strRow);
    }

    stream.close();
}


int main()
{
    try
    {
        // спросите у менторов в каком типе сохранены данные!!!
        using dtype = float;

        // не забудьте указать актуальные имена файлов!!!
        auto tuple = loadNpy<dtype>("example.npy");
        std::cout << std::endl;

        auto tableData = loadCsv<dtype>("example.csv");
        std::cout << std::endl;

        if ( tableData.size() >= 2 )
        {
            /*** Ваша магия с игреками ***/
            for ( size_t i = 0, size = tableData[1].size() ; i < size ; ++i )
            {
                tableData[1][i] = 100 + 50 * std::sin(i / 10.0);
            }
            /*** Конец магии ***/


            writeCsv(
                    tableData[0].cbegin(),
                    tableData[0].cend(),
                    tableData[1].cbegin(),
                    tableData[1].cend(),
                    "my_result.csv"
                    );
            std::cout << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
