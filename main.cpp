#include <iostream>
#include <pqxx/pqxx>
#include <Windows.h>

class Mydb {
public:
	Mydb() = default;
	//Запрещаем сонструктор копирования и оператор присваивания
	Mydb(const Mydb&) = delete;
	Mydb& operator=(const Mydb&) = delete;
	~Mydb() {
		if (conn) {
			conn->close(); delete conn;
		}
	}

	int create_db() {																					//Создание таблиц
		return exec_sql("create table if not exists users(id serial primary key, first_name varchar(50) not null, last_name varchar(50) not null); "
						"create table if not exists emails(uid integer not null references users(id) on delete cascade, email varchar(50) not null unique, CONSTRAINT email_pk PRIMARY KEY(uid, email)); "
						"create table if not exists phones(uid integer not null references users(id) on delete cascade, phone bigint not null unique, CONSTRAINT phone_pk PRIMARY KEY(uid, phone));");
	}

	int add_user(const std::string& first_name, const std::string& last_name) {							//Добавление нового пользователя
		return exec_sql("insert into users (first_name, last_name) values ('" + first_name + "', '" + last_name + "');");
	}

	//int add_email(const int user_id, const std::string& email) {}
	
	int add_phone(const int user_id, const long long int phone) {										//Добавление телефона у конкретного пользователя
		return exec_sql("insert into phones values (" + std::to_string(user_id) + ", " + std::to_string(phone) + ")");
	}
	
	int update_user(const int user_id, const std::string& first_name, const std::string& last_name) {	//Обновление данных пользователя
		if (!first_name.empty() || !last_name.empty()) {

			std::string sql_string = "update users ";
			if (!first_name.empty()) {
				sql_string += " set first_name = '" + first_name + "'";
				if (!last_name.empty()) {
					sql_string += ", last_name = '" + last_name + "'";
				}
			}
			else {
				sql_string += " set last_name = '" + last_name + "'";
			}
			sql_string += " where id = " + std::to_string(user_id);
			return exec_sql(sql_string);
		}
		else {
			std::cout << "Не указано ни имя, ни фамилия клиента!\n";
			return -1;
		}
	}
	
	int delete_phone(const int user_id) {																//Удаление всех телефонов пользователя
		return exec_sql("delete from phones where uid = " + std::to_string(user_id));
	}

	int delete_phone(const int user_id, const long long int phone) {									//Удаление конкретного телефона пользователя
		return exec_sql("delete from phones where uid = " + std::to_string(user_id) + " and phone = " + std::to_string(phone));
	}

	int delete_user(const int user_id) {																//Удаление пользователя
		return exec_sql("delete from users where id = " + std::to_string(user_id));
	};
	
	int search_user(const std::string& first_name, const std::string& last_name, const std::string& email, const long long int phone) {
		if (!conn || !conn->is_open()) {	//Если не подключены к бд, подключаемся
			if (!db_connect()) {
				std::cout << "Не удалось подключиться к базе данных." << std::endl;
				return -1;
			}
		}
		std::string sql_string = "select u.id, u.first_name, u.last_name, case when e.email is null then '' else e.email end, case when p.phone is null then '' else p.phone::varchar end from users u left join emails e on e.uid = u.id left join phones p on p.uid = u.id";
		if (!first_name.empty()) {
			sql_string += " where u.first_name = '" + first_name + "'";
		}
		if (!last_name.empty()) {
			if (sql_string.find("where") == std::string::npos) {
				sql_string += " where u.last_name = '" + last_name + "'";
			}
			else {
				sql_string += " or u.last_name = '" + last_name + "'";
			}
		}
		if (!email.empty()) {
			if (sql_string.find("where") == std::string::npos) {
				sql_string += " where e.email = '" + email + "'";
			}
			else {
				sql_string += " or e.email = '" + email + "'";
			}
		}
		if (phone != 0) {
			if (sql_string.find("where") == std::string::npos) {
				sql_string += " where p.phone = " + std::to_string(phone);
			}
			else {
				sql_string += " or p.phone = " + std::to_string(phone);
			}
		}
		pqxx::work tx{ *conn };
		int count = 0;
		try {

			for (auto [id, first_name, last_name, email, phone] : tx.query<int, std::string, std::string, std::string, std::string>(sql_string))
			{
				std::cout << std::to_string(id) << " " << first_name << " " << " " << last_name << " " << email << " " << phone << "\n";
				count++;
			}
		}
		catch (pqxx::sql_error e) {	//Не все проблемы подключения к базе здесь отлавливается, некоторые только в блоке catch ниже
			std::cout << "Execution error: " << e.what() << std::endl;
			return -1;
		}
		catch (std::exception e) {
			std::cout << "Ошибка выполнения SQL запроса: " << e.what() << std::endl;
			return -1;
		}
		return count;
	}
	
	pqxx::connection* conn{nullptr};

protected:
private:

	bool db_connect() {
		if (!conn) {
			try {
					conn = new pqxx::connection(
					"host=localhost "
					"port=5432 "
					"dbname=lesson_5 "
					"user=postgres "
					"password=123456");
			}
			catch (pqxx::sql_error e) {	//Не все проблемы подключения к базе здесь отлавливается, некоторые только в блоке catch ниже
				std::cout << "Connection error: " << e.what() << std::endl;
				return false;
			}
			catch (const std::exception e) {
				std::cout << "General error: " << e.what() << std::endl;
				return false;
			}
		}
		return conn->is_open();
	}

	int exec_sql(const std::string& sql_string) {
		if (!conn || !conn->is_open()) {	//Если не подключены к бд, подключаемся
			if (!db_connect()) {
				std::cout << "Не удалось подключиться к базе данных." << std::endl;
				return -1;
			}
		}
		pqxx::work tx{ *conn };
		try {
			pqxx::result res = tx.exec(sql_string);
			tx.commit();
			return res.affected_rows();
		}
		catch (pqxx::sql_error e) {	//Не все проблемы подключения к базе здесь отлавливается, некоторые только в блоке catch ниже
			std::cout << "Execution error: " << e.what() << "\nSQL команда: "<< sql_string << std::endl;
			return -1;
		}
		catch (const std::exception e) {
			std::cout << "General error: " << e.what() << "\nSQL команда: " << sql_string << std::endl;
			return -1;
		}
	}

};



int main()
{
	SetConsoleOutputCP(CP_UTF8);
	Mydb mydb;
	if (mydb.create_db() == -1) {
		std::cout << "Ошибка при создании таблиц\n";
	}
	if (mydb.add_user("First_name1", "Last_name1") > 0) {
		std::cout << "Успешное добавление пользователя First_name1 Last_name1.\n";
	}
	if (mydb.add_user("First_name2", "Last_name2") > 0) {
		std::cout << "Успешное добавление пользователя First_name2 Last_name2.\n";
	}
	if (mydb.add_user("First_name3", "Last_name3") > 0) {
		std::cout << "Успешное добавление пользователя  First_name3 Last_name3.\n";
	}
	if (mydb.update_user(2, "Super_first_name2", "Super_last_name2") > 0) {
		std::cout << "Успешное обновление данных для пользователя с id = 2.\n";
	}
	
	
	if (mydb.add_phone(1, 71111111111) > 0) {
		std::cout << "Успешное добавление номера телефона 71111111111 для пользователя с id = 1.\n";
	}
	if (mydb.add_phone(1, 72222222222) > 0) {
		std::cout << "Успешное добавление номера телефона 72222222222 для пользователя с id = 1.\n";
	}
	if (mydb.add_phone(1, 73333333333) > 0) {
		std::cout << "Успешное добавление номера телефона 73333333333 для пользователя с id = 1.\n";
	}
	if (mydb.add_phone(2, 74444444444) > 0) {
		std::cout << "Успешное добавление номера телефона 74444444444 для пользователя с id = 2.\n";
	}
	std::cout << "Ищем всех пользователей с именем First_name1\n";
	int processed_rows = mydb.search_user("First_name1", "", "", 0);
	if (processed_rows > 0) {
		std::cout << "Найдено " + std::to_string(processed_rows) + " пользователей\n";
	}
	std::cout << "Ищем всех пользователей с фамилией Last_name1\n";
	processed_rows = mydb.search_user("", "Last_name1", "", 0);
	if (processed_rows > 0) {
		std::cout << "Найдено " + std::to_string(processed_rows) + " пользователей\n";
	}
	std::cout << "Ищем всех пользователя с номером телефона 74444444444\n";
	processed_rows = mydb.search_user("", "", "", 74444444444);
	if (processed_rows > 0) {
		std::cout << "Найдено " + std::to_string(processed_rows) + " пользователей\n";
	}
	std::cout << "Ищем всех пользователей с именем  First_name1 или номером телефона 74444444444\n";
	processed_rows = mydb.search_user("First_name1", "", "", 74444444444);
	if (processed_rows > 0) {
		std::cout << "Найдено " + std::to_string(processed_rows) + " пользователей\n";
	}
	if (mydb.delete_phone(2, 74444444444) > 0) {
		std::cout << "Успешное удаление номера телефона 74444444444 для пользователя с id = 2.\n";
	}
	processed_rows = mydb.delete_phone(1);
	if (processed_rows > 0) {
		std::cout << "Успешное удаление всех телефонных номеров для пользователя с id = 1. Количество удаленных номеров: " + std::to_string(processed_rows) + "\n";
	}
	if (mydb.delete_user(3) > 0) {
		std::cout << "Успешное удаление пользователя с id = 3.\n";
	}

	return 0;
}