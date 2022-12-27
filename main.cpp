#include <iostream>
#include <string>
#include <queue>
#include <map>
#include <fstream>
#include <sstream>
#include <cstring>
#include <memory>
#include <functional>

/**
 * @brief Класс для кодирования/декодирования текста алгоритмом Хаффмана
 */
class Huffman
{
public:
    /**
     * @brief Нода для дерева Хаффмана
     * Для Ptr использовал shared_ptr чтобы не думать об утечке, shared_ptr сам все разрулит
     */
    struct Node
    {
        using Ptr = std::shared_ptr<Node>;

        char value;
        std::size_t freq;
        Node::Ptr left;
        Node::Ptr right;
    };

    // Компаратор для очереди с приоритетом.
    // С ним при вставке в очередь с приоритетом очередь автоматически будет сортироваться по возрастанию частоты символа
    struct Greater
    {
        bool operator()(const Node::Ptr& lhs, const Node::Ptr& rhs) const
        {
            return lhs->freq == rhs->freq ? lhs->value >= rhs->value : lhs->freq > rhs->freq;
        }
    };

    using PriorityQueue = std::priority_queue<Node::Ptr, std::vector<Node::Ptr>, Greater>;

    static std::string encode(const std::string& text);
    static std::string decode(const std::string& text);

private:
    static Node::Ptr generateHuffmanTree(const std::map<char, std::size_t>& freq);
    static std::map<char, std::string> generateHuffmanCodes(const Node::Ptr& root);
    static void generateHuffmanCodesImpl(const Node::Ptr& root, const std::string& repr, std::map<char, std::string>& huffmanCode);
    static std::map<char, std::size_t> getFreq(const std::string& text);

};

/**
 * @brief Закодировать текст
 * @param text Текст для закодирования
 * @return std::string Закодированный текст
 */
std::string Huffman::encode(const std::string& text)
{
    auto freq = getFreq(text);
    auto root = generateHuffmanTree(freq);

    auto huffmanCode = generateHuffmanCodes(root);

    // Вставляем заголовок с частотами
    std::string encoded = "het\n";

    for (const auto& f : freq)
        encoded += f.first + std::string(" ") + std::to_string(f.second) + '\n';

    encoded += "het\n";

    // Вставляем закодированный текст
    for (const auto& c : text)
        encoded += huffmanCode[c];

    return encoded;
}


/**
 * @brief Декодировать текст с помощью дерева Хаффмана
 * @param text Закодированный текст
 * @return std::string Декодированный текст
 */
std::string Huffman::decode(const std::string& text)
{
    // В начале должен быть заголовок с частотами
    if (text.substr(0, 3) != "het" || !std::isspace(text[3]))
        throw std::runtime_error("Can not decode not encoded text.");

    // Читаем частоты в карту
    std::map<char, std::size_t> freq;
    std::size_t i = 4;
    while (true)
    {
        char c = text[i];
        // Если дошли до конца - выходим
        if (c == 'h' && i + 2 < text.size() && text[i + 1] == 'e' && text[i + 2] == 't')
        {
            i += 4;
            break;
        }

        if (!std::isspace(text[i + 1]))
            throw std::runtime_error(std::string("Invalid encoded symbol: expected space after '") + c + "'.");

        i += 2;
        if (i >= text.size())
            throw std::runtime_error("Invalid encoded symbol.");

        char* e;
        freq[c] = std::strtoull(text.data() + i, &e, 10);

        i = e - text.data() + 1;
    }

    auto root = generateHuffmanTree(freq);

    const auto huffmanCode = generateHuffmanCodes(root);

    // Разворачиваем карту кодов (меняем местами ключ и значение)
    std::map<std::string, char> reversedHuffmanCode;
    for (const auto& c : huffmanCode)
        reversedHuffmanCode[c.second] = c.first;

    std::string decode;
    while (i < text.size())
    {
        // Пока не найдем соответсвие в reversedHuffmanCode набираем по символу.
        //Нашли - вставляем и по новой пока текст не кончится
        std::size_t j = i + 1;
        while (j <= text.size())
        {
            auto it = reversedHuffmanCode.find(text.substr(i, j - i));
            if (it != reversedHuffmanCode.end())
            {
                decode += it->second;
                break;
            }
            ++j;
        }
        i = j;
    }

    return decode;
}

/**
 * @brief Создать дерево Хаффмана
 * @param freq Карта частот символов
 * @return Huffman::Node::Ptr Корень дерева Хаффмана
 */
Huffman::Node::Ptr Huffman::generateHuffmanTree(const std::map<char, std::size_t>& freq)
{
    // Конвертируем карту частот в очередь с приоритетом
    PriorityQueue pq;
    {
        for (const auto& f : freq)
            pq.push(Node::Ptr(new Node{ f.first, f.second, nullptr, nullptr }));
    }

    // Берем два верхних элемента и создаем новый
    // С '\0' символом, частотой равной сумме частот взятых
    // И так пока в очереди не остается лишь один элемент - корень
    while (pq.size() > 1)
    {
        auto left = pq.top();
        pq.pop();

        auto right = pq.top();
        pq.pop();

        pq.push(Node::Ptr(new Node{ '\0', left->freq + right->freq, left, right }));
    }

    return pq.top();
}

/**
 * @brief Создать карту замен исходных символов на закодированные
 * @param root Корень дерева Хаффмана
 * @return std::map<char, std::string> Карта замен исходных символов на закодированные
 */
std::map<char, std::string> Huffman::generateHuffmanCodes(const Node::Ptr& root)
{
    std::map<char, std::string> huffmanCode;
    generateHuffmanCodesImpl(root, "", huffmanCode);

    return huffmanCode;
}

/**
 * @brief Создать карту замен исходных символов на закодированные
 * @param root Корень дерева Хаффмана
 * @param repr Текущий код для символа
 * @param huffmanCode Карта замен исходных символов на закодированные
 */
void Huffman::generateHuffmanCodesImpl(const Node::Ptr& root, const std::string& repr, std::map<char, std::string>& huffmanCode)
{
    // Рекурсивно проходимся по дереву и генерируем строку-замену для очередного символа
    // Если мы пошли влево - то к следующему коду приписываем '0', иначе '1'.
    // Когда у ноды нет потомков - записываем полученный код в карту замен
    if (root)
    {
        if (!root->left && !root->right)
            huffmanCode[root->value] = repr;

        generateHuffmanCodesImpl(root->left, repr + '0', huffmanCode);
        generateHuffmanCodesImpl(root->right, repr + '1', huffmanCode);
    }
}

/**
 * @brief Получить карту частот символов в тексте
 * @param text входной текст
 * @return std::map<char, std::size_t> карта частот символов в тексте
 */
std::map<char, std::size_t> Huffman::getFreq(const std::string& text)
{
    std::map<char, std::size_t> freq;
    for (const auto& c : text)
        ++freq[c];

    return freq;
}

/**
 * @brief Прочитать весь файл
 * @param filename Имя файла
 * @return std::string Содержимое файла
 * @throw std::runtime_error Если не удалось открыть файл с заданным именем
 */
std::string readFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open())
        throw std::runtime_error("Can not open \"" + filename + "\"");

    std::stringstream ss;
    ss << ifs.rdbuf();

    return ss.str();
}

/**
 * @brief Написать в файл
 * @param filename Имя файла
 * @param text Текст для записи
 * @throw std::runtime_error Если не удалось открыть файл с заданным именем
 */
void writeFile(const std::string& filename, const std::string& text)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open())
        throw std::runtime_error("Can not open \"" + filename + "\"");

    ofs << text;
}

int main()
{
    std::cout << "1 - encode, 2 - decode\n";

    char c;
    std::cin >> c;
    if (c == '1')
    {
        std::string filename;
        std::cout << "Filename: ";
        std::cin >> filename;

        const auto input = readFile(filename);

        const auto encoded = Huffman::encode(input);

        writeFile("encoded.txt", encoded);
    }
    else if (c == '2')
    {
        std::string filename;
        std::cout << "Filename: ";
        std::cin >> filename;

        const auto encoded = readFile(filename);

        const auto decoded = Huffman::decode(encoded);

        writeFile("decoded.txt", decoded);
    }
    else
    {
        throw std::runtime_error("Invalid usage.");
    }

    return 0;
}