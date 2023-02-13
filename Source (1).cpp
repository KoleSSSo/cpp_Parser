#include <iostream>
#include <tuple>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

//строка -> любой тип
template <typename T>
T setArgument(std::string inputValue)
{
	T arg;  std::stringstream buffer;

	buffer << inputValue;
	buffer >> arg;

	return arg;
}


//вывод I - кол-во аргументов
template <typename Ch, typename Tr, size_t I, typename... Args>
class TuplePrinter
{
public:
	static void print(std::basic_ostream<Ch, Tr>& os, const std::tuple<Args...>& t)
	{
		TuplePrinter<Ch, Tr, I - 1, Args...>::print(os, t);
		if (I < sizeof...(Args))
			os << " ";
		os << std::get<I>(t);
	}
};

//и закончили. для нулевого аргумента
template <typename Ch, typename Tr, typename... Args>
class TuplePrinter<Ch, Tr, 0, Args...>
{
public:
	static void print(std::basic_ostream<Ch, Tr>& os, const std::tuple<Args...>& t)
	{
		os << std::get<0>(t);
		return;
	}
};

// оператор << будет запускать TurplePrinter
template <typename Ch, typename Tr, typename... Args>
auto operator<<(std::basic_ostream<Ch, Tr>& os, const std::tuple<Args...>& t)
{
	TuplePrinter<Ch, Tr, sizeof...(Args) - 1, Args...>::print(os, t);
}

template<typename... Args>
class CSVParser
{
public:
	CSVParser(std::ifstream& file, size_t skipLines, char delimeter = ',', char safe = '\"') : file_name(file) //this->file = file для потоков не работает
	{
		this->delimiter = delimeter;		//разделитель
		this->saveSymbol = safe;			//символ экранирования
		this->read_rows = 0;				//сколько уже считали
		this->skip_lines = skipLines;		//пропуск строк

		if (!file.is_open()) throw std::runtime_error("File is not opened!"); //не существующий файл

		std::string __trash;				//пропуск строк
		while (this->read_rows < this->skip_lines)
		{
			std::getline(file, __trash);
			this->read_rows++;
		}

		//поеъхали
		this->buffer = this->getString();
	}

	~CSVParser() = default;

	std::tuple<Args...> getString()
	{
		std::string	read_string;
		std::vector<std::string> splited_string;

		std::vector<std::string> output = getData();

		if (this->file_name.eof()) return std::tuple<Args...>();

		//Возващаем последнене значение из вектора (лямбда функция в си))))
		auto flip = [&output]() -> std::string
		{
			std::string s = output.back();
			output.pop_back();
			return s;
		};

		if (output.size() != sizeof...(Args))	throw std::runtime_error("Invalid string");					//в строке больше или меьше столбцов
		if (isSafe == true)						throw std::runtime_error("The save data wasn't closed!");	//если не нашли закрывающего знака экранизации

		return std::tuple<Args...>(setArgument<Args>(flip())...);
	}

	//класс iterator для того чтобы писать как в питоне например. auto x : p (x in p)
	class CSVInputIterator
	{
	public:
		using iterator_category = std::input_iterator_tag;
		using value_type = std::tuple<Args...>;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		CSVInputIterator(CSVParser<Args...>* _object = nullptr) : object(_object) {};		//конец файла
		CSVInputIterator(const CSVInputIterator&) = default;
		CSVInputIterator& operator=(const CSVInputIterator&) = default;
		//меняет местами a и b 
		friend void swap(CSVInputIterator& a, CSVInputIterator& b) { std::swap(a.object, b.object); }
		//сравнение
		friend bool operator==(CSVInputIterator& a, CSVInputIterator& b) { return a.object == b.object; }
		friend bool operator!=(CSVInputIterator& a, CSVInputIterator& b) { return a.object != b.object; }
		//проверка на конец файла
		value_type operator*() { if (!object->isEof) return object->buffer; }
		//следующее значение
		CSVInputIterator& operator++()
		{
			object->buffer = object->getString();

			if (object->isEof == true) object = nullptr;

			return *this;
		}

		CSVParser<Args...>* object;
	};

	CSVInputIterator begin() { return CSVInputIterator(this); }
	CSVInputIterator end() { return CSVInputIterator(); }

private:
	size_t	read_rows;
	size_t	skip_lines;
	std::ifstream& file_name;
	std::vector<char> dividers;
	std::tuple<Args...> buffer;

	char delimiter;
	char saveSymbol;
	bool isSafe = false;	//флаг экранизации
	bool isEof = false;		//флаг конца файла

	std::vector<std::string> getData()
	{
		std::string	read_string;
		std::vector<std::string> splitted_string;
		std::vector <std::string> __data;			//поделенная строка с учётом экранизации. (то что мы будем возвращать)

		std::getline(this->file_name, read_string);

		if (file_name.eof())
		{
			this->isEof = true;
			return std::vector<std::string>();
		}

		this->read_rows++;

		size_t start = 0, end = 0;
		while ((start = read_string.find_first_not_of(delimiter, end)) != std::string::npos)
		{
			end = read_string.find(delimiter, start);
			splitted_string.push_back(read_string.substr(start, end - start));
		}

		//экранизация
		std::string value;
		for (auto x : splitted_string)
		{
			value += x;

			if (x.find(saveSymbol) != std::string::npos)
			{
				isSafe = !isSafe;
			}

			if (!isSafe)
			{
				__data.push_back(value);
				value = "";
			}

			if (isSafe)
			{
				value += delimiter;
			}
		}

		if (this->file_name.eof()) this->isEof = true;

		return __data;
	}
};


int main()
{
	std::ifstream fin("input.csv");
	CSVParser<int, std::string, double, int, int, int, double, int, double, int, std::string> p(fin, 3, ',', '/');

	for (auto x : p) //auto beg = p.begin(); beg != p.end(); beg++
	{
		std::cout << x;
		std::cout << std::endl;
	}

	return 0;
}
