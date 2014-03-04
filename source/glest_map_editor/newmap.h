#ifndef NEWMAP_H
#define NEWMAP_H

#include <QDialog>

namespace Ui {
class NewMap;
}

class NewMap : public QDialog
{
    Q_OBJECT

public:
    explicit NewMap(QWidget *parent = 0);
    ~NewMap();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::NewMap *ui;
};

#endif // NEWMAP_H
