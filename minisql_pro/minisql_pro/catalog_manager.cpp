// miniSQL.cpp: 定义控制台应用程序的入口点。
//
#include"catalog_manager.h"

void catalog_ReadIndex(catalog_index_table *index,int &number)//用于将一个索引表对应文件的内容读到内部结构中，返回数量
{
	FILE* fp;
	char *buf = (char*)malloc(sizeof(char) * 1000);
	std::string file_name = "./catalog/catalog_index_table";
	int count = 0;
	fopen_s(&fp, file_name.c_str(), "r+");
	if (fp == NULL)
	{
		fopen_s(&fp, file_name.c_str(), "w+");
		fprintf_s(fp, "0 ");
		number = 0;
		fclose(fp);
		index = NULL;
		return;
	}
	fscanf_s(fp, "%d", &number);
	for(int i=0;i<number;i++)
	{
		fscanf_s(fp, "%s", buf,1000);
		index[i].index_name = buf;
		fscanf_s(fp, "%s", buf,1000);
		index[i].table_name = buf;
	}
	fclose(fp);
	return;
}

void catalog_WriteIndex(catalog_index_table *index,int number)//用于写一个索引表对应文件
{
	FILE* fp;
	std::string file_name = "./catalog/catalog_index_table";
	fopen_s(&fp, file_name.c_str(), "w+");
	fprintf_s(fp, "%d ", number);
	for (int i = 0; i < number; i++)
	{
		fprintf_s(fp, "%s %s ", index[i].index_name.c_str(), index[i].table_name.c_str());
	}
	fclose(fp);
	return;
}

int ReadTable(catalog_Table &table)//用于将一个表文件的内容读到内部结构中
{
	FILE* fp;
	std::string file_name = "./catalog/" + table.table_name + ".dat";
	int name_length = 0, type_length = 0,index_length=0,index_num=0;
	int unique, index;
	char *buf;
	buf = (char*)malloc(sizeof(char));
	fopen_s(&fp, file_name.c_str(), "r+");
	if (fp == NULL)
	{
		std::cout << "Table not exist!" << std::endl;//表不存在！
		return catalog_table_not_exist;
	}
	fscanf_s(fp, "%hd %hd ", &table.attr.num, &table.attr.primary_key);//写入table的属性数和主键位数
	for (int i = 0; i < table.attr.num; i++)
	{
		fscanf_s(fp, "%d",&name_length);
		fgetc(fp);
		buf = (char*)realloc(buf, sizeof(char)*(name_length+1));
		fread_s(buf, name_length + 1,1, name_length,fp);
		buf[name_length] = '\0';
		table.attr.name[i] = buf;

		fscanf_s(fp, "%d", &type_length);
		fgetc(fp);
		buf=(char*)realloc(buf, sizeof(char)*(type_length + 1));
		fread_s(buf, type_length + 1, 1, type_length, fp);
		buf[type_length] = '\0';
		table.attr.type[i] = buf;

		fscanf_s(fp, "%hd %d %d", &table.attr.size[i], &unique,&index);
		table.attr.unique[i] = unique == 0 ? false : true;
		table.attr.has_index[i] = index == 0 ? false : true;
		fgetc(fp);
		if (index != 0)//存在索引
		{
			fscanf_s(fp, "%d", &index_length);
			fgetc(fp);
			buf = (char*)realloc(buf, sizeof(char)*(index_length + 1));
			fread_s(buf, index_length + 1, 1, index_length, fp);
			buf[index_length] = '\0';
			table.index.indexname[index_num] = buf;
			table.index.location[index_num] = i;
			index_num++;
		}
		table.index.num = index_num;
			
	}
	fclose(fp);
	return 0;
}
void WriteTable(catalog_Table table)//用于将一个表的内容写出到表文件中
{
	FILE* fp;
	std::string file_name = "./catalog/" + table.table_name + ".dat";
	int index=0;
	fopen_s(&fp, file_name.c_str(), "w+");
	fprintf_s(fp, "%hd %hd ", table.attr.num,table.attr.primary_key);//输出table的属性数和主键位数
	for (int i = 0; i < table.attr.num; i++)
	{
		fprintf_s(fp, "%d %s%d %s%hd %d %d ", table.attr.name[i].length(), table.attr.name[i].c_str(),
			table.attr.type[i].length(),table.attr.type[i].c_str(),
			table.attr.size[i], (int)(table.attr.unique[i]), (int)(table.attr.has_index[i]));
		if (table.attr.has_index[i] == true)
		{
			fprintf_s(fp, "%d %s", table.index.indexname[index].length(), table.index.indexname[index].c_str());
			index++;
		}
	}
	fclose(fp);
}

int catalog_CreateTable(std::string table_name, struct Attribute attribute, catalog_Index index)
{
	catalog_Table tmp;
	int attribute_num=0;
	std::string path= "./catalog/" + table_name + ".dat";
	tmp.table_name = table_name;
	tmp.attr = attribute;
	tmp.index = index;
	if (_access(path.c_str(), 0) == 0)//如果文件存在
	{
		std::cout << "The table already exists!" << std::endl;
		return catalog_table_exist;//返回表已存在错误
	}
	else
		WriteTable(tmp);
	std::cout<<"Create table successfully!"<<std::endl;
	return 0;
}

int catalog_DropTable(std::string table_name)
{
	std::string file_name = "./catalog/" + table_name + ".dat";
	if (_access(file_name.c_str(), 0) == 0)//如果文件存在
	{
		catalog_DropTable_delete_indexs(table_name);
		remove(file_name.c_str());//删除表文件
		std::cout << "Drop table completed!" << std::endl;
	}
	else
	{
		std::cout << "Table not exist!" << std::endl;
		return catalog_table_not_exist;
	}
	return 0;
}

int catalog_CreateIndex(std::string table_name, std::string attr_name, std::string index_name)
{
	catalog_Table tmp;
	tmp.table_name = table_name;
	catalog_index_table search[Max_index_num];
	int number;
	if (ReadTable(tmp))
	{
		return catalog_table_not_exist;//return 1
	}
	for (int i = 0; i < tmp.attr.num; i++)
	{
		if (tmp.attr.name[i] == attr_name)
		{//找到目标属性，添加索引
			if (tmp.attr.has_index[i] == true)
			{//该属性已存在索引
				std::cout << "Index already exists!" << std::endl;
				return catalog_index_exist;
			}
			catalog_ReadIndex(search, number);
			for (int i = 0; i < number; i++)
				if (search[i].index_name == index_name)
				{
					std::cout << "Index name already exists!" << std::endl;
					return catalog_index_name_exist;
				}

			search[number].table_name = table_name;
			search[number].index_name = index_name;
			catalog_WriteIndex(search, number + 1);
			tmp.attr.has_index[i] = true;
			tmp.index.indexname[tmp.index.num] = index_name;
			tmp.index.location[tmp.index.num] = i;
			tmp.index.num++;
			WriteTable(tmp);//将改写后的table写回文件
			return 0;//索引添加完毕返回
		}
	}
	std::cout << "Attibute not exists!" << std::endl;
	return catalog_attr_not_exist;//属性不存在错误
}

int catalog_DropIndex(std::string index_name)
{
	std::string table_name = catalog_Index_to_table_name(index_name);
	if (table_name == "")
	{
		std::cout << "index_not_exist!" << std::endl;
		return catalog_index_not_exist;
	}
	return(catalog_DropIndex(table_name, index_name));
}
int catalog_DropIndex(std::string table_name, std::string index_name)
{
	catalog_Table tmp;
	tmp.table_name = table_name;
	catalog_index_table search[Max_index_num];
	int number;
	if (ReadTable(tmp))
	{
		return catalog_table_not_exist;//表不存在错误
	}
	for (int i = 0; i < tmp.index.num; i++)
	{
		if (tmp.index.indexname[i] == index_name)
		{//找到目标索引，进行删除
			catalog_ReadIndex(search, number);
			for (int i = 0; i < number; i++)
			{
				if (search[i].index_name == index_name)
				{
					for (int j = i; j < number - 1; j++)
						search[j] = search[j + 1];
					break;
				}
			}
			catalog_WriteIndex(search, number - 1);

			tmp.attr.has_index[ tmp.index.location[i] ] = false;
			for (int j = i; j < tmp.index.num - 1; j++)
			{
				tmp.index.indexname[j] = tmp.index.indexname[j + 1];
				tmp.index.location[j] = tmp.index.location[j + 1];
			}
			tmp.index.num--;
			WriteTable(tmp);//将改写后的table写回文件
			return 0;//删除完毕返回
		}
	}
	std::cout << "Index not exists!" << std::endl;
	return catalog_index_not_exist;//return 2
}

std::vector<std::string> catalog_get_index(const std::string &table_name)
{
	std::vector<std::string> result;
	catalog_Table table;
	table.table_name = table_name;
	result.clear();
	if (ReadTable(table))
	{
		std::cout << "表不存在！" << std::endl;
		return result;
	}
	for (int i = 0; i < table.index.num; i++)
		result.push_back(table.index.indexname[i]);
	return result;
}

std::vector<attr_type> catalog_get_attr(const std::string &table_name)
{
	std::string name;
	std::vector<attr_type> result;
	attr_type tmp;
	catalog_Table table;
	Attribute attr;
	table.table_name = table_name;
	result.clear();
	if (ReadTable(table))
	{
		std::cout << "表不存在！" << std::endl;
		return result;
	}
	attr = table.attr;
	for (int i = 0; i < attr.num; i++)
	{
		tmp.attr_name = attr.name[i];
		tmp.has_index = attr.has_index[i];
		tmp.is_primary_key = attr.primary_key == i ? true : false;
		tmp.is_unique = attr.unique[i];
		tmp.size = attr.size[i];
		tmp.type_name = attr.type[i];
		result.push_back(tmp);
	}
	return result;
}

std::string catalog_Index_to_table_name(std::string index_name)
{
	catalog_index_table search[Max_index_num];
	int number;
	catalog_ReadIndex(search, number);
	for (int i = 0; i < number; i++)
		if (search[i].index_name == index_name)
			return search[i].table_name;
	return "";
}

void catalog_DropTable_delete_indexs(std::string table_name)
{
	catalog_index_table search[Max_index_num];
	int number;
	catalog_ReadIndex(search, number);
	for (int i = 0; i < number; i++)
	{
		if (search[i].table_name == table_name)
		{
			for (int j = i; j < number - 1; j++)
				search[j] = search[j + 1];
			number--;
		}
	}
	catalog_WriteIndex(search, number);
}