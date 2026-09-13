# 게임 데이터 안내

[기획 허브](../docs/README.md) · [데이터 필드 명세](../docs/DATA_SCHEMA.md)

이 폴더에는 UE 임포트용 DataTable8개와 GameRules1개가 있습니다. 현재 수치와 생성 공식의 **편집 원본은 [build-design-data.mjs](../tools/build-design-data.mjs)**입니다. JSON만 수동으로 바꾸면 재생성 때 덮어써집니다.

| 생성 결과 | 내용 |
|---|---|
| DT_Units.json | 수호자 스탯·등급·스킬 참조 |
| DT_Skills.json | 발동·피해·범위·효과 |
| DT_Recipes.json | 신화 제작 재료 |
| DT_EnemyTypes.json | 일반 적·보스 스탯 |
| DT_Waves.json | 웨이브 생성·보상 |
| DT_SpawnProfiles.json | 적 유형 수량 배분 |
| DT_SummonProfiles.json | 기본·보장·P0 확률 |
| DT_Upgrades.json | 강화 비용·효과 |
| GameRules.json | 공용 설정과 P0 오버라이드 |

수치를 변경하면 관련 기능 명세의 설명·예시, QA 기대 결과, RulesVersion을 함께 검토합니다. 생성 뒤 validate-design-data를 실행합니다. 데이터 검증 보고서는 자동 생성되며 직접 수정하지 않습니다.

UE DataTable의 수동 편집 결과를 원본으로 삼지 않습니다. 에디터에서 실험한 변경은 원본에 반영한 뒤 다시 임포트합니다. 기획자용 스프레드시트 도입 시에는 원본을 그쪽으로 명시적으로 이관하고 생성 스크립트를 내보내기 도구로 바꿉니다.
