#include "newmap.h"
#include "ui_newmap.h"

NewMap::NewMap(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMap)
{
    ui->setupUi(this);
}

NewMap::~NewMap()
{
    delete ui;
}

void NewMap::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
