$ANDROID_BUILD_TOP = ./
$OUT = /../../../out/target/product/sp9832e_1h10_go
charger_source=$(cd "$(dirname $0)";pwd)
charger_gcno=$OUT"/obj/NATIVE_TESTS/charge_arm_test_intermediates"

cd $charger_gcno
mkdir $charger_source/data
cp *.gcno $charger_source/data
cp ./dotdot/*.gcno $charger_source/data

cd $charger_source/data
#get gcda file
lcov -c -d ./ -o app.info
lcov --extract app.info '*vendor/modules/charge/charge/*' -o result.info
genhtml result.info -o ./output/
