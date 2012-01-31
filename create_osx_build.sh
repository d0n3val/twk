#!/bin/sh

./bin/platypus -P share/TouchyWarehouseKeeper.platypus TouchyWarehouseKeeper.app
zip -rm twk`date +%Y_%m_%d`-osx.zip TouchyWarehouseKeeper.app

