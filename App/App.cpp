#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <cmath>
#include <tuple>
#include <fstream>

//Gnuplot lib
#include "gnuplot-cpp/gnuplot_i.hpp"

// хэдеры для работы с .npy
#include <array> // в npy.hpp забыли заинклюдить
#include "npy.hpp"

// хэдеры для работы с .csv
#include "csv2/reader.hpp"
#include "csv2/writer.hpp"


/* вывод части вектора на экран */
template <typename DTYPE>
void printVector(const  std::vector<DTYPE>& data)
{
    std::cout << "[";

    const size_t nVisible = 5;
    const size_t size = data.size();
    size_t i = 0;

    for (; i < size && i < nVisible; ++i)
    {
        std::cout << data[i];
        if (i != size - 1)
        {
            std::cout << ", ";
        }
    }

    if (nVisible * 2 < size)
    {
        std::cout << "... , ";
    }

    i = ((i + nVisible >= size) ? i : size - nVisible);
    for (; i < size; ++i)
    {
        std::cout << data[i];
        if (i != size - 1)
        {
            std::cout << ", ";
        }
    }
    std::cout << "]";
}


/* пример загрузки npy */
template <typename DTYPE>
std::tuple<std::vector<DTYPE>, std::vector<unsigned long>, bool> loadNpy(const std::string& fileName)
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
    for (size_t i = 0, size = shape.size(); i < size; ++i)
    {
        std::cout << shape[i];
        if (i != size - 1)
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
std::vector< std::vector<DTYPE> > loadCsv(const std::string& fileName)
{
    csv2::Reader<
        csv2::delimiter<','>,
        csv2::quote_character<'"'>,
        csv2::first_row_is_header<true>,
        csv2::trim_policy::trim_whitespace
    > csvReader;

    std::vector< std::vector<DTYPE> > dataColumns;

    if (csvReader.mmap(fileName))
    {
        const size_t rowCount = csvReader.rows();
        const size_t columnCount = csvReader.cols();
        //        const auto header = csv.header();

                // таблица результатов, записанная по колонкам
        dataColumns.resize(columnCount);

        // считывание из файла и запись в таблицу результатов
        size_t iRow = 1; // хедер пропускаем
        auto row = csvReader.begin();
        for (; iRow < rowCount; ++row, ++iRow)
        {
            size_t iColumn = 0;
            auto cell = (*row).begin();
            /* NOTE если делать через range-for или полностью через итераторы,
             * то не считывается последний столбец, если в нем есть пустые ячейки */
            for (; iColumn < columnCount; ++cell, ++iColumn)
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
        for (const auto& oneColumn : dataColumns)
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
void writeCsv(IterX xBegin, IterX xEnd, IterY yBegin, IterY yEnd, const std::string& fileName)
{
    std::ofstream stream(fileName);
    csv2::Writer< csv2::delimiter<','> > csvWriter(stream);

    // запись хэдера
    std::vector<std::string> strRow = { "x", "y" };
    csvWriter.write_row(strRow);

    // запись построчно
    for (; xBegin != xEnd && yBegin != yEnd; ++xBegin, ++yBegin)
    {
        strRow[0] = std::to_string(*xBegin);
        strRow[1] = (std::isnan(*yBegin) ? "" : std::to_string(*yBegin));
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
        auto [data,shape,isF] = loadNpy<dtype>("hackathon-three-cities_all_data_L1.npy");
        std::cout << std::endl;

        //std::ofstream out("temp.csv");
        //Gnuplot gp;
        //for (int i = 0; i < shape[0]; i++)
        //{
        //    for (int j = 0; j < shape[1]; j++)
        //    {
        //        out << i << '\t' << j << '\t' << data[i * shape[1] + j] << '\n';
        //        out << data[i * shape[1] + j] << ';';
        //                std::cout << i << '\t' << j << '\t' << data[i*shape[1]+j] << '\n';
        //    }
        //    out << '\n';
        //}
        //gp << "set hidden3d";
        //gp << "set dgrid3d 50,50 qnorm 2";
        //gp.plotfile_xyz("temp.txt",1,2,3,"");

        auto tableData = loadCsv<dtype>("hackathon-three-cities_L1_horizons_train.csv");
        std::cout << std::endl;

        if (tableData.size() >= 2)
        {
            int row = 2;
            float maxDivY = 0;
            int p = 0;
            /*** Ваша магия с игреками ***/
            for (size_t i = 0, size = tableData[1].size() - 1; i < size; ++i)
            {
                if (abs(tableData[row][i + 1] - tableData[row][i]) >= maxDivY)
                    maxDivY = abs(tableData[row][i + 1] - tableData[row][i]);
                //std::cout << i << '\t' << tableData[2][i] << '\n';
                //tableData[1][i] = 100 + 50 * std::sin(i / 10.0);
            }
            
            //std::cout << ceil(maxDivY) << '\n';
            //std::cout << p << '\n';
            //p = 500;
            //maxDivY = 2.;
            while (tableData[2][p] != tableData[2][p]) { tableData[2][p] = 0; p++; }
            while (tableData[2][p] == tableData[2][p]) p++;
            p--;
            while (tableData[row][p+1] != tableData[row][p+1] && p < 1451)
            //while(p < 1451)
            {
                float newY = roundf(tableData[row][p]);
                for (int i = 0; i <= ceil(maxDivY); i++)
                {
                    float maxLH = sqrtf(powf(data[p * shape[1] + roundf(tableData[row][p])], 2.) + 1.);
                    float minLH = sqrtf(powf(data[p * shape[1] + roundf(tableData[row][p])], 2.) + 1.);
                    float dy = 0.1;
                    float h = 0.;
                    float l = 0.;
                    float lh = 0.;
                    // left p * shape[1] - i - 1
                    for (float d = -1.; d <= 0.; d += dy)
                    {
                        h = ((data[p * shape[1] + roundf(tableData[row][p]) - i + 1] -
                            data[p * shape[1] + roundf(tableData[row][p]) - i]) / (dy)) * (d)+
                            data[p * shape[1] + roundf(tableData[row][p]) - i];
                        l = sqrtf((i - d) * (i - d) + 1.);
                        lh = sqrtf(l * l + h * h);
                        if (lh > maxLH)
                        {
                            maxLH = lh;
                            newY = roundf(tableData[row][p]) + d;
                        }
                        /*if (lh < minLH)
                        {
                            minLH = lh;
                            newY = roundf(tableData[2][p]) + d;
                        }*/
                    }
                    // right p * shape[1] + i + 1
                    for (float d = 0.; d <= 1.; d += dy)
                    {
                        h = ((data[p * shape[1] + roundf(tableData[row][p]) + i] -
                            data[p * shape[1] + roundf(tableData[row][p]) + i - 1]) / (dy)) * (d)+
                            data[p * shape[1] + roundf(tableData[row][p]) + i - 1];
                        l = sqrtf((i - d) * (i - d) + 1.);
                        lh = sqrtf(l * l + h * h);
                        if (lh > maxLH)
                        {
                            maxLH = lh;
                            newY = roundf(tableData[row][p]) + d;
                        }
                        /*if (lh < minLH)
                        {
                            minLH = lh;
                            newY = roundf(tableData[2][p]) + d;
                        }*/
                    }
                }
                p++;
                tableData[row][p] = newY;
            }
            /*** Конец магии ***/


            writeCsv(
                tableData[0].cbegin(),
                tableData[0].cend(),
                tableData[row].cbegin(),
                tableData[row].cend(),
                "my_result.csv"
            );
            std::cout << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
