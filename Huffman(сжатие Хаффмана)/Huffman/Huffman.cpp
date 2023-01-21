// Huffman.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <list>
#include <map>
using namespace std;
struct Symbol //символ
{
	char sym; //значение
	int count,bit_count; //частота и число бит в коде
	long long code; //код
	int id;
	string str_code;
	Symbol() {

	}
	Symbol(char sym, int count, int id) {
		this->sym = sym; this->count = count; this->id = id;
	}
	Symbol(char sym,int bit_count, long long code) {
		this->sym = sym; 
		this->code = code; 
		this->bit_count = bit_count;
	}
	bool operator<(Symbol sym) {
		return count < sym.count;
	}
	bool operator>(Symbol sym)
	{
		return count > sym.count;
	}
	char cut_bit(long long value,int bit_count, int pos,int cut_len)
	{
		return (value >> (bit_count - (pos + cut_len))) & ((0xFF)>>(8- cut_len));
	}
	friend bool operator >(const Symbol& a, const  Symbol& b);
	void WriteCompressedBits(char*& arr, int &number_bit_in_byte, int &number_current_byte) //запись кода в массив байт
	{
		int byte_pushed = (number_bit_in_byte + bit_count) / 8;
		int origin_bit = 0; //ShowCode();
		for (int i = 0; i <= byte_pushed && origin_bit<= bit_count-1; i++)
		{
			int free_bit_current_byte_count = 8 - number_bit_in_byte; //число свободных бит в текущем байте
			int bit_lost = bit_count - origin_bit;
			int cut_len = (free_bit_current_byte_count< bit_lost)*(free_bit_current_byte_count - bit_lost)+ bit_lost;//число вырезанных бит
			
			char cut_bits = cut_bit(code,bit_count, origin_bit, cut_len); //вырезанные биты
			//cout << "-----------------------"<< endl;
			//cout << "code "<< code << endl;
			//cout <<"origin_bit " << origin_bit  << endl;
			//cout <<"number_bit_in_byte " << number_bit_in_byte << endl;
			//cout << "cut_len " << cut_len <<endl;
			//cout << "cut_bits " << hex << (int)cut_bits << endl;
			//cout << "free_bit_current_byte_count " << free_bit_current_byte_count << endl;
			origin_bit += cut_len;//новая позиция бита для обрезки битов
			cut_bits <<= (free_bit_current_byte_count-cut_len);//заталкивание слева битов в байт
			//cout << "cut_bits " << hex << (int)cut_bits << endl;
			//cout << "arr[number_current_byte + i]" << hex << (arr[number_current_byte + i] & 0xFF) << endl;
			arr[number_current_byte + i] |= cut_bits;//биты записаны в байт
			//cout << "arr[number_current_byte + i]" << hex << (arr[number_current_byte + i] & 0xFF) << endl;
			//cout << "-----------------------" << endl;
			number_bit_in_byte = (number_bit_in_byte + cut_len) % 8;
		}
		number_current_byte+= byte_pushed;
	}
	void Save(ofstream& f)
	{
		f.write(&sym, 1); //запоминание символа
		f.write((char*)&bit_count, 4); //запоминание длины кода
		f.write((char*)&code, sizeof(long long)); //запоминание кода
	}
	void ShowCode()
	{
		cout << sym << " ";
		long long copy_code = code;
		for (int i = bit_count-1; i>=0; i--)
		{
			cout << ((copy_code>>i) & 0x1);
		}
		cout << endl;
	}
};
bool operator >(const Symbol& a, const  Symbol& b) //Перегрузка >
{
	return a.count > b.count;
}


class TreeNode //узел дерева, абстрактный
{

protected:
	int count; //частота
public:
	TreeNode(){}
	TreeNode(int count) { this->count = count; }
	int Count() { return count; }
	virtual void BuildCode(long long digit_code,int bit_count) = 0; //рекурсивное построение кода
};

class SymbolTreeNode :public TreeNode //листовая вершина дерева
{
	Symbol *symbol;
public:
	SymbolTreeNode(Symbol *symbol, int count) :TreeNode(count)
	{ this->symbol = symbol;}
	virtual void BuildCode(long long digit_code, int bit_count) //рекурсивное построение кода
	{
		symbol->bit_count = (bit_count>0)*bit_count;
		symbol->code = digit_code;
	}
};

class ParentTreeNode:public TreeNode //предки листьевых деревье
{
protected:
	TreeNode* left, *right; //ссылки на левые и правые дочерние вершины

public:
	ParentTreeNode(TreeNode* left, TreeNode* right,int count):TreeNode(count)
	{
		this->left = left; this->right = right; 
	}
	virtual void BuildCode(long long digit_code, int bit_count) //рекурсивное построение кода
	{
		left->BuildCode(digit_code<<1, bit_count+1);
		right->BuildCode((digit_code << 1)| 0x1, bit_count + 1);
	}
};

class HuffmamTree //дерево хаффмана
{
	list<Symbol> *list_symbol; //список символов
	TreeNode* root; //корень дерева
	Symbol* HashSymbol[256]; //массив
public:
	HuffmamTree() {}
	HuffmamTree(list<Symbol>* list_symbol)
	{
		this->list_symbol = list_symbol;
		//BuildTree();
	}
	void BuildTree()
	{
		list<TreeNode*> list_nodes;
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			Symbol* sym = &*iter;
			HashSymbol[sym->sym + 127] = sym;
			list_nodes.push_back(new SymbolTreeNode(sym, iter->count));//инициализация листьев дерева
		}
		int count = list_nodes.size();
		root = *list_nodes.begin();
		//list_nodes.sort(/*greater<TreeNode>()*/);
		for (int i = 0; i < count - 1; i++)//построение вершин дерева
		{
			list<TreeNode*>::iterator min_iter_1, min_iter_2;
			FindMinPair(list_nodes, min_iter_1, min_iter_2);//поиск минимальных вершин без родителей
			TreeNode* min_node1 = *min_iter_1, * min_node2 = *min_iter_2;
			int sum = min_node1->Count()+ min_node2->Count();
			//cout << min_node1->Count() << " " << min_node2->Count() << endl;
			list_nodes.erase(min_iter_1); list_nodes.erase(min_iter_2);//удаление двух вершин из списка
			root = new ParentTreeNode(min_node2, min_node1, sum);//новая безродная вершина дерева в списке
			list_nodes.push_back(root);
		}
		BuildCode(); //построение оптимального кода
	}
	void FindMinPair(list<TreeNode*> &list_nodes,list<TreeNode*>::iterator& min_iter_1, list<TreeNode*>::iterator& min_iter_2) //поиск пар минимальных безродных вершин
	{
		//cout << "---------------------\n";
		auto first = list_nodes.begin(), second= ++list_nodes.begin();
		TreeNode* first_node = *first, *second_node=*second;
		if (first_node->Count() <= second_node->Count())
		{
			min_iter_1 = first;
			min_iter_2 = second;
		}
		else
		{
			min_iter_2 = first;
			min_iter_1 = second;
		}
		for (auto iter = ++second; iter != list_nodes.cend(); iter++)
		{
			TreeNode* node = *iter, * min_node1 = *min_iter_1, * min_node2 = *min_iter_2;
			//cout << node->Count() << endl;
			if (node->Count() <= min_node1->Count())
			{
				if (node->Count() == min_node1->Count() && min_node1->Count()<= min_node2->Count())
				{
					min_iter_2 = min_iter_1;
				}
				min_iter_1 = iter;
				
			}
			if (node->Count() < min_node2->Count() && node->Count()> min_node1->Count())
				min_iter_2 =iter;
		}
	}
	void BuildCode()
	{
		root->BuildCode(0, 0);
	}
	int CompressedBitSize()
	{
		int bit_count = 0;
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			bit_count += iter->bit_count * iter->count;
		}
		return bit_count;
	}
	char* CreateCompressedBitArray(char* file_bit,long size, long &compess_size)
	{
		int bit_count = CompressedBitSize();
		int tail_bit_count = bit_count % 8;
		int byte_count = bit_count / 8+(tail_bit_count>0);
		char* compressed_array = new char[byte_count];
		memset(compressed_array, 0, byte_count);
		int current_byte = 0, number_bit_in_byte=0;
		for (int i = 0; i < size; i++)
		{
			int id = file_bit[i] + 127;
			//cout << file_bit[i]<<"|" << HashSymbol[id]->sym<< endl;
			HashSymbol[id]->WriteCompressedBits(compressed_array, number_bit_in_byte, current_byte);
		}
		compess_size = byte_count;
		return compressed_array;
	}
	void PackFile(string filename, char* file_bit, long size)
	{
		ofstream fout;
		fout.open(filename, ios::binary | ios::out);
		fout.write((char*)&size,4);//запоминанение размера файла
		int count = list_symbol->size();
		fout.write((char*)&count, 4);//запоминание числа различных символов
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)//запоминание ключевой информации
		{
			Symbol* sym = &*iter;
			sym->Save(fout);
		}
		long size_comressed = 0;
		char* compressed = CreateCompressedBitArray(file_bit, size, size_comressed);
		/*for (int i = 0; i < size_comressed; i++)
			cout << hex << (compressed[i]  & 0xFF);
		cout << endl;*/
		fout.write(compressed, size_comressed);
		fout.close();
		/*for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			iter->ShowCode();
		}*/
		cout << "Энтропия: " << Entropy(size) << endl;
		cout << "Минимальный возможный размер: " << CompressedBitSize() << " бит\n";
	}
	double Entropy(long file_size)
	{
		double value = 0;
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			double p = (iter->count + 0.0) / file_size;
			value += -log2(p)*p;
		}
		return value;
	}
};

struct PairKeySymbol
{
	int len;
	long long code;
public:
	PairKeySymbol(long long code, int len) :len(len), code(code) {}
};

class Board
{
	HuffmamTree tree;
	char* FileByte;
	long file_size;
	map<long long, char> map_for_code;

public:
	Board() {}
	void ReadFile(string filename)
	{
		list<Symbol> *list_symbol=new list<Symbol>();
		ifstream ifs(filename, ios::binary | ios::ate);
		ifstream::pos_type pos = ifs.tellg();
		file_size = pos;
		FileByte = new char[file_size];//массив байт файла
		//MessageBox::Show(length + "");
		ifs.seekg(0, ios::beg);
		ifs.read(FileByte, file_size);//чтение в массив байт
		int* Alphabet = new int[256];//частоты всех возможных символов
		memset(Alphabet, 0, sizeof(int) * 256);//обнуление массива
		for (int i = 0; i < file_size; i++)//подсчёт частот символов
			Alphabet[127 + FileByte[i]]++;
		int id = -1;
		for (int i = 0; i < 256; i++)//инициализация всех символов файла
		{
			int count = Alphabet[i];
			if (count > 0)
			{
				id++;
				list_symbol->push_back(Symbol(i - 127, count, id));
			}
		}
		ifs.close();
		tree= HuffmamTree(list_symbol);
		tree.BuildTree();
	}
	void PackFile(string filename)
	{
		ReadFile(filename);
		tree.PackFile(filename + ".huf", FileByte, file_size);
	}
	void ReadCompressedFile(string filename)
	{
		map_for_code.clear();
		list<Symbol>* list_symbol = new list<Symbol>();
		ifstream ifs(filename, ios::binary);
		ifs.read((char*)&file_size, 4);//чтение исходного размера
		int symbols_count = 0;
		ifs.read((char*)&symbols_count, 4);//чтение числа символов
		//cout<< file_size <<"---"<< symbols_count << "->\n";
		long long unit = 1;
		for (int i = 0; i < symbols_count; i++)//чтение ключевой информации
		{
			long long code; char s; int len;
			ifs.read(&s, 1);
			ifs.read((char*)&len, 4);
			ifs.read((char*)&code, sizeof(long long));
			list_symbol->push_back(Symbol(s,len, code));
			map_for_code[code|(unit<<len)]=s;
		}
		long cur_pos = ifs.tellg();
		//cout << "|-->" <<dec<< ifs.tellg() << endl;
		ifs.seekg(ifs.cur, ios_base::end);
		long compessed_size = ifs.tellg(); compessed_size -= (cur_pos+1);
		ifs.seekg(cur_pos, ios::beg);
		FileByte = new char[compessed_size];//массив байт файл
		ifs.read(FileByte, compessed_size);//чтение в массив байт
		ifs.close();
		//cout <<"-->"<<dec<< compessed_size<<endl;
		/*for (int i = 0; i < compessed_size; i++)
			cout << hex << (FileByte[i] & 0xFF);*/
		//cin >> compessed_size;
		tree = HuffmamTree(list_symbol);
		/*for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			iter->ShowCode();
		}*/
	}
	void UnPack(string filename)
	{
		ReadCompressedFile(filename);
		int number_current_byte = 0, number_bit_in_byte=0; 
		char* unpacked = new char[file_size];
		for (int i = 0; i < file_size; i++)
		{
			long long code = 1, bit_count = 0;
			bool not_exists = true;
			auto iter = map_for_code.end();
			while (not_exists)
			{
				char byte = FileByte[number_current_byte];
				code <<= 1; bit_count++;
				code |= (byte >> (7 - number_bit_in_byte)) & 0x1;
				iter = map_for_code.find(code);
				not_exists = iter == map_for_code.end();
				number_current_byte += (number_bit_in_byte == 7);
				number_bit_in_byte = (number_bit_in_byte + 1) % 8;
			}
			//cout<<hex<<code<<" " << iter->second << endl;
			//cout << number_current_byte <<" " << (int)FileByte[number_current_byte] << endl;
			unpacked[i] =iter->second;
		}
		ofstream fout;
		fout.open(filename+".bak", ios::binary | ios::out);
		fout.write(unpacked, file_size);
		fout.close();
	}
};



int main()
{
	setlocale(LC_ALL, "Russian");
	Board board;
	board.PackFile("input.txt");
	board.UnPack("input.txt.huf");
	system("pause");
}
