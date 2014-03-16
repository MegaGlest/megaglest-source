#ifndef NEWMAP_H
#define NEWMAP_H

#include <QDialog>

class Renderer;

namespace Ui {
class NewMap;
}

class NewMap : public QDialog
{
    Q_OBJECT

public:
    explicit NewMap(Renderer *renderer, QWidget *parent = 0);
    ~NewMap();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::NewMap *ui;
    Renderer *renderer;

private slots:
    void create();
};

#endif // NEWMAP_H
