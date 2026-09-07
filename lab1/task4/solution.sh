#!/bin/bash

pack_proj_to_tar() {
  echo "Запаковываем директории proj в архив..."
  tar -czf proj.tar.gz ./proj # что делаает флаг -f
}

create_out_directory() {
  if [ -d "out" ]; then
    echo "Файл out в данной директории $(pwd) уже существует."
    echo "Перезаписать директории? [Y/n]"

    read answer
    if [[ $answer == "Y" || $answer == "y" ]]; then
      echo "Удаляем директории out..."
      # Найти исходный код rm и найти как она обрабатывает входной массив флагов
      rm -fr out
      mkdir out
      echo "Директория пересоздана."
    else
      echo "Отмена перезаписи паки out."
      return 0
    fi
  else
    mkdir out
    echo "Директория out создана."
    return 1
  fi
}

copy_tar_to_out() {
  echo "Копирование proj.tar.gz в out..."
  cp proj.tar.gz out/proj.tar.gz
}

decompress_tar_in_out() {
  echo "Распаковка proj.tar.gz в out."
  tar -C out/ -xzf out/proj.tar.gz
}

delete_tar_in_out() {
  echo "Удаляем proj.tar.gz из out..."
  rm -rf out/proj.tar.gz
}


pack_proj_to_tar
if ! create_out_directory; then
  exit 1
fi
copy_tar_to_out
decompress_tar_in_out
delete_tar_in_out
mkdir out/include out/src out/shared out/build
mv out/proj/*.h out/include/ 2>/dev/null
mv out/proj/*.hpp out/shared/ 2>/dev/null
mv out/proj/*.cpp out/src/ 2>/dev/null
g++ out/src/*.cpp -Iout/include -Iout/shared -o out/build/main # Как в gcc передать несколько папок

./out/build/main <<< "30 12" > stdout.txt #Зачем ставим точку перед и с чем это связанно








