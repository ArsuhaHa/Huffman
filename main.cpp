#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cstring>
#include <memory>

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
            return lhs->freq > rhs->freq;
        }
    };

    using PriorityQueue = std::priority_queue<Node::Ptr, std::vector<Node::Ptr>, Greater>;

    static std::string encode(const std::string& text, Node::Ptr* codeTree = nullptr);
    static std::string decode(const std::string& text, const Node::Ptr& codeTree);

private:
    static Node::Ptr generateHuffmanTree(const std::string& text);
    static std::unordered_map<char, std::string> generateHuffmanCodes(const Node::Ptr& root);
    static void generateHuffmanCodesImpl(const Node::Ptr& root, const std::string& repr, std::unordered_map<char, std::string>& huffmanCode);
    static std::unordered_map<char, std::size_t> getFreq(const std::string& text);

};

/**
 * @brief Закодировать текст
 * @param text Текст для закодирования
 * @param codeTree Опциональный выходной параметр. В него поместится указатель на корень дерева Хаффмана
 * @return std::string Закодированный текст
 */
std::string Huffman::encode(const std::string& text, Node::Ptr* codeTree)
{
    auto root = generateHuffmanTree(text);

    auto huffmanCode = generateHuffmanCodes(root);

    std::string encoded;
    for (const auto& c : text)
        encoded += huffmanCode[c];

    if (codeTree)
        *codeTree = root;

    return encoded;
}

/**
 * @brief Декодировать текст с помощью дерева Хаффмана
 * @param text Закодированный текст
 * @param codeTree Корень дерева Хаффмана
 * @return std::string Декодированный текст
 */
std::string Huffman::decode(const std::string& text, const Node::Ptr& codeTree)
{
    if (!codeTree)
        throw std::runtime_error("Can not decode \"" + text + "\" with nullptr code tree.");

    // Проходим по тексту и дереву, собирая код из текста, и, если в дереве нашли символ - вставляем в decoded
    // Прим: aaaabc
    //              -----
    //         0/          1\\
    //      a, 4            ---
    //                  0/      1\\
    //                b, 1      c, 1
    // (Т, е a - 0, b - 10, c - 11)
    // Зашифрованное: 00001011
    // Берем первый 0, по карте идем влево, берем след 0, по карте идти некуда -> вставляем 'a' и по новой.
    // Так пока не дойдем до 1. От корня вправо, след символ - 0, влево, след символ 1, право пусто, добавляем 'b'
    // Имеем '1' - от корня вправо, далее '1' - еще раз вправо, дальше '\0', смотрим право, нет его, дописываем 'c'
    std::string decode;
    for (std::size_t i = 0; i < text.size();)
    {
        auto temp = codeTree;
        while (true)
        {
            if (text[i] == '0')
            {
                if (temp->left)
                {
                    temp = temp->left;
                }
                else
                {
                    decode += temp->value;
                    break;
                }
            }
            else
            {
                if (temp->right)
                {
                    temp = temp->right;
                }
                else
                {
                    decode += temp->value;
                    break;
                }
            }
            ++i;
        }
    }

    return decode;
}

/**
 * @brief Создать дерево Хаффмана
 * @param text Текст, по которому будет созданно дерево
 * @return Huffman::Node::Ptr Корень дерева Хаффмана
 */
Huffman::Node::Ptr Huffman::generateHuffmanTree(const std::string& text)
{
    // Конвертируем карту частот в очередь с приоритетом
    PriorityQueue pq;
    {
        const auto freq = getFreq(text);
        for (const auto& f : freq)
            pq.push(Node::Ptr(new Node{ f.first, f.second, nullptr, nullptr }));
    }

    // Берем два верхних элемента и создаем новый
    // С '\0' символом, частотой равной сумме частот взятых
    // И так пока в очереди не остается лишь один элемент - корень
    while (pq.size() != 1)
    {
        auto left = std::move(pq.top());
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
 * @return std::unordered_map<char, std::string> Карта замен исходных символов на закодированные
 */
std::unordered_map<char, std::string> Huffman::generateHuffmanCodes(const Node::Ptr& root)
{
    std::unordered_map<char, std::string> huffmanCode;
    generateHuffmanCodesImpl(root, "", huffmanCode);

    return huffmanCode;
}

/**
 * @brief Создать карту замен исходных символов на закодированные
 * @param root Корень дерева Хаффмана
 * @param repr Текущий код для символа
 * @param huffmanCode Карта замен исходных символов на закодированные
 */
void Huffman::generateHuffmanCodesImpl(const Node::Ptr& root, const std::string& repr, std::unordered_map<char, std::string>& huffmanCode)
{
    // Рекурсивно проходимся по дереву и генерируем строку-замену для очередного символа
    // Если мы пошли влево - то к следующему коду приписываем '0', иначе '1'.
    // Когда у ноды нет потомков - записываем полученный код в карту замен
    if (root)
    {
        if (!root->left && !root->right)
        {
            huffmanCode[root->value] = repr;
            return;
        }

        generateHuffmanCodesImpl(root->left, repr + '0', huffmanCode);
        generateHuffmanCodesImpl(root->right, repr + '1', huffmanCode);
    }
}

/**
 * @brief Получить карту частот символов в тексте
 * @param text входной текст
 * @return std::unordered_map<char, std::size_t> карта частот символов в тексте
 */
std::unordered_map<char, std::size_t> Huffman::getFreq(const std::string& text)
{
    std::unordered_map<char, std::size_t> freq;
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
    const auto input = readFile("input.txt");

    Huffman::Node::Ptr root;
    const auto encoded = Huffman::encode(input, &root);

    writeFile("encoded.txt", encoded);

    const auto decoded = Huffman::decode(encoded, root);

    writeFile("decoded.txt", decoded);

    return 0;
}