#ifndef SQLITE_DATABASE_H_
#define SQLITE_DATABASE_H_

#include <QDebug>

#include <sqlite3.h>
#include <stdexcept>
#include <vector>

struct ObjectFeatures {
  ObjectFeatures(const std::string &obj_name, const double hue,
                 const double area, const double round);

  std::string m_obj_name;
  double m_hue;
  double m_area;
  double m_round;
};

class SqliteDatabase {
public:
  SqliteDatabase(const std::string &db_name);
  ~SqliteDatabase();

  void openDbConnection();
  void closeDbConnection();

  void execQuery(const std::string &query) const;
  void execInsertSqlQuery(const std::string &img_name,
                          const std::string &obj_name, const double hue,
                          const double area, const double round) const;
  std::vector<ObjectFeatures> extractFeaturesFromDb() const;
  int objectNameCount(const std::string &name) const;
  std::string objectNameUsingKnn(const ObjectFeatures &new_obj) const;

private:
  const std::string m_db_name;
  sqlite3 *m_db;
};

#endif // SQLITE_DATABASE_H_
