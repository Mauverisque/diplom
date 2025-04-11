#include <QApplication>

#include "main_window.h"
#include "sqlite_database.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  SqliteDatabase db("../knn_features.db");

  MainWindow main_window(db);
  main_window.show();

  return app.exec();
}
