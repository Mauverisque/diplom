#include "sqlite_database.h"

ObjectFeatures::ObjectFeatures(const std::string &obj_name, const double hue,
                               const double area, const double round)
    : m_obj_name(obj_name), m_hue(hue), m_area(area), m_round(round) {}

SqliteDatabase::SqliteDatabase(const std::string &db_name)
    : m_db_name(db_name), m_db(nullptr) {}

SqliteDatabase::~SqliteDatabase() { closeDbConnection(); }

void SqliteDatabase::openDbConnection() {
  if (sqlite3_open(m_db_name.c_str(), &m_db) != SQLITE_OK) {
    qDebug() << "WARNING: Database connection failed";
  } else {
    qDebug() << "Database opened successfully";
    execQuery("CREATE TABLE IF NOT EXISTS Features (Image TEXT UNIQUE, "
              "Object TEXT, Hue DOUBLE, Area DOUBLE, Round DOUBLE);");
  }
}

void SqliteDatabase::closeDbConnection() {
  if (m_db) {
    if (sqlite3_close(m_db) != SQLITE_OK) {
      qDebug() << "WARNING: unable to close database";
    } else {
      qDebug() << "Database closed successfully";
      m_db = nullptr;
    }
  }
}

void SqliteDatabase::execQuery(const std::string &query) const {
  char *err_msg = nullptr;
  if (sqlite3_exec(m_db, query.c_str(), nullptr, nullptr, &err_msg) !=
      SQLITE_OK) {
    qDebug() << err_msg;
    sqlite3_free(err_msg);
  }
}

void SqliteDatabase::execInsertSqlQuery(const std::string &img_name,
                                        const std::string &obj_name,
                                        const double hue, const double area,
                                        const double round) const {
  const char *query{"INSERT INTO Features (Image, Object, Hue, Area, Round) "
                    "VALUES (?, ?, ?, ?, ?) ON CONFLICT(Image) DO UPDATE SET "
                    "(Object, Hue, Area, Round) = (EXCLUDED.Object, "
                    "EXCLUDED.Hue, EXCLUDED.Area, EXCLUDED.Round);"};
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    qDebug() << "WARNING: Failed to prepare statement:" << sqlite3_errmsg(m_db);
    return;
  }

  sqlite3_bind_text(stmt, 1, img_name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, obj_name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_double(stmt, 3, hue);
  sqlite3_bind_double(stmt, 4, area);
  sqlite3_bind_double(stmt, 5, round);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    qDebug() << "WARNING: Execution failed:" << sqlite3_errmsg(m_db);
  } else {
    qDebug() << "Features upserted successfully:" << img_name << obj_name;
  }

  sqlite3_finalize(stmt);
}

std::vector<ObjectFeatures> SqliteDatabase::extractFeaturesFromDb() const {
  std::vector<ObjectFeatures> objects;

  const char *query{"SELECT Object, Hue, Area, Round FROM Features"};
  sqlite3_stmt *stmt{};

  if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const std::string obj_name{
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))};
      const double hue{sqlite3_column_double(stmt, 1)};
      const double area{sqlite3_column_double(stmt, 2)};
      const double round{sqlite3_column_double(stmt, 3)};

      objects.emplace_back(obj_name, hue, area, round);
    }
  }
  sqlite3_finalize(stmt);

  return objects;
}

int SqliteDatabase::objectNameCount(const std::string &name) const {
  const char *query{"SELECT COUNT(*) FROM Features WHERE Object = ?"};
  sqlite3_stmt *stmt{};

  int res{0};

  if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    qDebug() << "WARNING: Failed to prepare statement:" << sqlite3_errmsg(m_db);
    return res;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    qDebug() << "WARNING: Query execution failed:" << sqlite3_errmsg(m_db);
  } else {
    res = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);

  return res;
}

std::string
SqliteDatabase::objectNameUsingKnn(const ObjectFeatures &new_obj) const {
  const std::vector<ObjectFeatures> objects{extractFeaturesFromDb()};

  if (objects.size() == 0) {
    return "";
  }

  std::vector<std::pair<std::string, double>> distances;

  for (const auto &object : objects) {
    const std::string name{object.m_obj_name};
    const double hue{object.m_hue};
    const double area{object.m_area};
    const double round{object.m_round};

    // Calculate using Eucledian distance formula
    const double distance{sqrt(pow((hue - new_obj.m_hue), 2) +
                               pow((area - new_obj.m_area), 2) +
                               pow((round - new_obj.m_round), 2))};

    distances.emplace_back(name, distance);
  }

  std::sort(distances.begin(), distances.end(),
            [](std::pair<std::string, double> left,
               std::pair<std::string, double> right) {
              return left.second < right.second;
            });

  std::vector<std::pair<std::string, int>> names_count;

  qDebug() << objectNameCount(distances[0].first) << "nearest neighbours";
  for (int i = 0; i != objectNameCount(distances[0].first); ++i) {
    qDebug() << distances[i].first << distances[i].second;
    const std::string name{distances[i].first};
    const auto iter{std::find_if(
        names_count.begin(), names_count.end(),
        [name](std::pair<std::string, int> n) { return n.first == name; })};
    if (iter == names_count.end()) {
      names_count.emplace_back(name, 1);
    } else {
      (*iter).second += 1;
    }
  }

  const auto res{std::max_element(
      names_count.begin(), names_count.end(),
      [](std::pair<std::string, int> left, std::pair<std::string, int> right) {
        return left.second < right.second;
      })};

  return (*res).first;
}
